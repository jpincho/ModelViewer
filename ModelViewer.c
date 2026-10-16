#include <Platform/Platform.h>
#include <Platform/PlatformTime.h>
#include <Platform/Logger.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include <CrossUI/CrossUI.h>
#include "EventDispatcher.h"
#include "ModelViewer.h"
#include "Camera.h"
#include "CameraInput.h"
#include "Utils.h"
#include "ResourceLoading/ResourceHub.h"
#include "ResourceLoading/AABB.h"
#if defined (PLATFORM_COMPILER_GNU)
#include <alloca.h>
#endif

static crFramebufferHandle MainWindowFramebuffer = NULL;
static CameraData MainCamera = { 0 };
static bool ShouldQuit = false;
static char *ModelFilename = NULL;
static float ModelScale = 1.0f;
static bool ForceRenderCommandUpdate = false;
static ivec2 LastMousePosition;
static bool HaveLastMousePosition = false;
static ModelRenderData ModelCommands = { 0 };

#define TEST_FILENAME MODEL_PATH "/GLTF/Character1/untitled.gltf"

static ResourceHandle LoadModel ( const char *Filename, const char *ResourceName, ModelRenderData *RenderData );
static void ResetCamera ( CameraData *Camera );

static void KeyStateChanged_Callback ( const crWindowHandle Handle, const crKeyCode KeyCode, const bool State )
	{
	static bool LeftCtrlPressed = false, RightCtrlPressed = false;
	static bool OPressed = false;

	edSendKeyStateChangedEvent ( Handle, KeyCode, State );
	switch ( KeyCode )
		{
		case crKeyCode_Escape:
			{
			ShouldQuit = true;
			break;
			}
		case crKeyCode_LeftControl:
			{
			LeftCtrlPressed = State;
			break;
			}
		case crKeyCode_RightControl:
			{
			RightCtrlPressed = State;
			break;
			}
		case crKeyCode_LeftAlt:
		case crKeyCode_RightAlt:
			{
			CameraInput_SetMouseViewEnabled ( State );
			break;
			}
		case crKeyCode_O:
			{
			if ( ( ( LeftCtrlPressed ) || ( RightCtrlPressed ) ) && ( State == true ) && ( OPressed == false ) )
				{
				char *SelectedFilename = NULL;
				SelectedFilename = cuiFileDialog ( NULL, "Load model file", "*.gltf;*.glb", false, false );
				if ( SelectedFilename == NULL )
					{
					LOG_ERROR ( "Error selecting file" );
					break;
					}

				ResourceHub_DestroyAll();
				DestroyModelRenderData ( &ModelCommands );

				ResourceHandle NewModelHandle = LoadModel ( SelectedFilename, "Model", &ModelCommands );
				free ( SelectedFilename );
				if ( NewModelHandle == ResourceHandle_Invalid )
					{
					LOG_ERROR ( "Unable to load model file" );
					break;
					}
				ForceRenderCommandUpdate = true;
				}
			OPressed = State;
			break;
			}
		case crKeyCode_R:
			{
			ResetCamera ( &MainCamera );
			break;
			}
		}
	}

static void WindowClosed_Callback ( const crWindowHandle Handle )
	{
	ShouldQuit = true;
	}

static void MouseMoved_Callback ( const crWindowHandle Handle, const ivec2 NewPosition )
	{
	ivec2 Delta = { 0, 0 };
	if ( HaveLastMousePosition )
		{
		Delta.x = NewPosition.x - LastMousePosition.x;
		Delta.y = NewPosition.y - LastMousePosition.y;
		}
	LastMousePosition = NewPosition;
	HaveLastMousePosition = true;
	edSendMouseMovedEvent ( Handle, NewPosition, Delta );
	}

static void WindowResized_Callback ( const crWindowHandle Handle, const uvec2 NewSize )
	{
	if ( MainWindowFramebuffer != NULL )
		{
		crDeleteFramebuffer ( MainWindowFramebuffer );
		MainWindowFramebuffer = NULL;
		}
	uvec2 ClientAreaDimensions;
	crGetWindowClientAreaDimensions ( Handle, &ClientAreaDimensions );
	crFramebufferDescriptor FramebufferDescriptor = { 0 };
	FramebufferDescriptor.ColorAttachments = 1;
	FramebufferDescriptor.ColorAttachmentFormat = crPixelFormat_RedGreenBlueAlpha8888;
	FramebufferDescriptor.DepthEnabled = true;
	FramebufferDescriptor.DepthFormat = crPixelFormat_DepthComponent;
	FramebufferDescriptor.Dimensions = ClientAreaDimensions;
	FramebufferDescriptor.ClearDepth = 1.0f;
	math_vec4_set ( &FramebufferDescriptor.ClearColor, 0.5f, 0.5f, 0.5f, 1.0f );

	MainWindowFramebuffer = crCreateFramebuffer ( FramebufferDescriptor );

	SetCameraPerspectiveParameters ( &MainCamera, MATH_PI_2, ( float ) ClientAreaDimensions.x / ( float ) ClientAreaDimensions.y, 0.1f, 1000.0f );
	UpdateCameraMatrices ( &MainCamera );
	ForceRenderCommandUpdate = true;
	}

static bool InitializeRenderer ( void )
	{
	// Create rendering window and framebuffer
	crRendererConfiguration Configuration = { 0 };
	crSetConfigurationToDefault ( &Configuration );
	Configuration.InitialWindowDescriptor.Fullscreen = false;
	Configuration.InitialWindowDescriptor.Position.x = 100;
	Configuration.InitialWindowDescriptor.Position.y = 100;
	Configuration.InitialWindowDescriptor.Size.x = 1920;
	Configuration.InitialWindowDescriptor.Size.y = 1080;
	Configuration.InitialWindowDescriptor.Resizable = true;
	Configuration.InitialWindowDescriptor.Title = "Model viewer";
	Configuration.DepthBits = 24;
	Configuration.VSyncEnabled = true;
	if ( crInitialize ( Configuration ) == false )
		return false;

	crWindowManagerCallbacks Callbacks = { 0 };
	Callbacks.WindowClosed = WindowClosed_Callback;
	Callbacks.KeyStateChanged = KeyStateChanged_Callback;
	Callbacks.WindowResized = WindowResized_Callback;
	Callbacks.MouseMoved = MouseMoved_Callback;
	crSetWindowManagerCallbacks ( Callbacks );

	uvec2 ClientAreaDimensions;
	crGetWindowClientAreaDimensions ( crGetMainWindowHandle(), &ClientAreaDimensions );

	crFramebufferDescriptor FramebufferDescriptor = { 0 };
	FramebufferDescriptor.ColorAttachments = 1;
	FramebufferDescriptor.ColorAttachmentFormat = crPixelFormat_RedGreenBlueAlpha8888;
	FramebufferDescriptor.DepthEnabled = true;
	FramebufferDescriptor.DepthFormat = crPixelFormat_DepthComponent;
	FramebufferDescriptor.Dimensions = ClientAreaDimensions;
	FramebufferDescriptor.ClearDepth = 1.0f;
	math_vec4_set ( &FramebufferDescriptor.ClearColor, 0.5f, 0.5f, 0.5f, 1.0f );

	MainWindowFramebuffer = crCreateFramebuffer ( FramebufferDescriptor );
	if ( !MainWindowFramebuffer )
		return false;
	return true;
	}

static bool InitializeCamera ( CameraData *Camera )
	{
	uvec2 WindowSize;
	CameraInput_Initialize ( Camera );
	crGetWindowClientAreaDimensions ( crGetMainWindowHandle(), &WindowSize );

	SetCameraPerspectiveParameters ( Camera, MATH_PI_2, ( float ) WindowSize.x / ( float ) WindowSize.y, 0.1f, 1000.0f );
	SetCameraOrientation ( Camera, MATH_QUAT_IDENTITY );
	SetCameraPosition ( Camera, ( vec3 )
		{
		0.0f, 0.0f, 4.0f
		} );
	UpdateCameraMatrices ( Camera );
	CameraInput_SetMouseViewEnabled ( false );

	return true;
	}

static ResourceHandle LoadModel ( const char *Filename, const char *ResourceName, ModelRenderData *RenderData )
	{
	LOG_DEBUG ( "Loading model from '%s'", Filename );

	ResourceHandle Handle = LoadModelFileIntoResourceHub ( Filename, ResourceName );
	if ( Handle == ResourceHandle_Invalid )
		{
		LOG_ERROR ( "Unable to load model" );
		return ResourceHandle_Invalid;
		}

	ModelDescriptor *Descriptor = ResourceHub_GetModelDescriptor ( Handle );
	if ( Descriptor == NULL )
		{
		LOG_ERROR ( "Unable to retrieve model descriptor" );
		return ResourceHandle_Invalid;
		}
	AABB_InitEmpty ( &Descriptor->AABBox );

	// Calculate the bounding boxes for all the nodes in this model, using the tree and the matrices
	if ( Descriptor->NodeCount != 0 )
		{
		mat4 *NodeWorldMatrices = alloca ( Descriptor->NodeCount * sizeof ( mat4 ) );
		ComputeMeshTreeNodeWorldMatrices ( Descriptor->Nodes, Descriptor->NodeCount, NodeWorldMatrices );
		for ( unsigned NodeIndex = 0; NodeIndex < Descriptor->NodeCount; ++NodeIndex )
			{
			for ( unsigned MeshIndex = 0; MeshIndex < Descriptor->Nodes[NodeIndex].MeshIndexCount; ++MeshIndex )
				{
				const unsigned MeshArrayIndex = Descriptor->Nodes[NodeIndex].MeshIndices[MeshIndex];
				if ( MeshArrayIndex >= Descriptor->MeshCount )
					continue;
				AABB TransformedBox;
				AABB_Transform ( &TransformedBox, &Descriptor->Meshes[MeshArrayIndex].BoundingBox, &NodeWorldMatrices[NodeIndex] );
				AABB_Join ( &Descriptor->AABBox, &Descriptor->AABBox, &TransformedBox );
				}
			}
		}
	LOG_DEBUG ( "Model loaded" );
	ModelData *LoadedModelData = ResourceHub_GetModelData ( Handle );

	if ( BuildModelRenderData ( RenderData, LoadedModelData ) == false )
		return ResourceHandle_Invalid;

	return Handle;
	}

static void ResetCamera ( CameraData *Camera )
	{
	uvec2 WindowSize;
	crGetWindowClientAreaDimensions ( crGetMainWindowHandle(), &WindowSize );

	SetCameraPerspectiveParameters ( Camera, MATH_PI_2, ( float ) WindowSize.x / ( float ) WindowSize.y, 0.1f, 1000.0f );
	SetCameraOrientation ( Camera, MATH_QUAT_IDENTITY );
	SetCameraPosition ( Camera, ( vec3 )
		{
		0.0f, 0.0f, 4.0f
		} );
	}

#include <unistd.h>

int main ( int argc, char *argv[] )
	{
	BaseTransformStruct ModelBaseTransform = { 0 };
	ResourceHandle LoadedModelHandle = ResourceHandle_Invalid;

	if ( InitializeRenderer() == false )
		return -1;

	if ( InitializeCamera ( &MainCamera ) == false )
		return -1;
	ResetCamera ( &MainCamera );

	if ( cuiInitializeWithArgs ( argc, argv ) == false )
		return -1;

	char cwd[500];
	if ( getcwd ( cwd, sizeof ( cwd ) ) != NULL )
		{
		printf ( "Current working dir: %s\n", cwd );
		}
	math_vec3_set ( &ModelBaseTransform.Position, 0.0f, 0.0f, 0.0f );
	ModelBaseTransform.Orientation = MATH_QUAT_IDENTITY;
	UpdateCachedValues ( &ModelBaseTransform );
	LoadedModelHandle = LoadModel ( TEST_FILENAME, "Model", &ModelCommands );
	if ( LoadedModelHandle == ResourceHandle_Invalid )
		return -1;

	ModelDescriptor *Descriptor = ResourceHub_GetModelDescriptor ( LoadedModelHandle );
	if ( Descriptor == NULL )
		return -1;

	AdjustToFit ( Descriptor->AABBox, &MainCamera, &ModelScale );

	//ModelScale = CalculateScaleToFit2 ( Descriptor->AABBox, MainCamera.Transform.Position.z, MainCamera.PerspectiveParameters.FOV, MainCamera.PerspectiveParameters.AspectRatio );
	UpdateModelRenderData ( &ModelCommands, &ModelBaseTransform, ModelScale, MainCamera.CachedValues.ViewMatrix, MainCamera.CachedValues.ProjectionMatrix );


	double LastEndFrameSeconds = Platform_GetPerformanceCurrentTime();
	double TotalDelta = 0.0;
	bool ModelTransformDirty = true, CameraTransformDirty = true;
	while ( ShouldQuit == false )
		{
		// World tick
		double Seconds = Platform_GetPerformanceCurrentTime();
		double TimeDelta = Seconds - LastEndFrameSeconds;
		if ( TimeDelta == 0.0 )
			TimeDelta = 0.01;
		LastEndFrameSeconds = Seconds;
		TotalDelta += TimeDelta;

		CameraInput_ProcessCameraMovement ( TimeDelta );

		ModelTransformDirty = ModelBaseTransform.CachedValues.Dirty;
		CameraTransformDirty = MainCamera.Transform.CachedValues.Dirty;

		// Updating rendering data, if needed
		if ( CameraTransformDirty )
			UpdateCameraMatrices ( &MainCamera );
		if ( ModelTransformDirty )
			UpdateCachedValues ( &ModelBaseTransform );
		if ( ( ModelTransformDirty || CameraTransformDirty || ForceRenderCommandUpdate ) && ( ModelCommands.Commands != NULL ) )
			{
			UpdateModelRenderData ( &ModelCommands, &ModelBaseTransform, ModelScale, MainCamera.CachedValues.ViewMatrix, MainCamera.CachedValues.ProjectionMatrix );
			ForceRenderCommandUpdate = false;
			ModelTransformDirty = CameraTransformDirty = false;
			}

		// Actually rendering the frame
		char TempString[256];
		snprintf ( TempString, 256, "%f %f %f", MainCamera.Transform.Position.x, MainCamera.Transform.Position.y, MainCamera.Transform.Position.z );
		crSetWindowTitle ( crGetMainWindowHandle(), TempString );

		crStartRenderToWindow ( crGetMainWindowHandle() );
		crClearFramebufferWithDefaultValues ( MainWindowFramebuffer );
		for ( unsigned Index = 0; Index < ModelCommands.Count; ++Index )
			crRunCommand ( ModelCommands.Commands[Index] );

		crDisplayFramebuffer ( MainWindowFramebuffer, crGetMainWindowHandle() );
		crDisplayWindow ( crGetMainWindowHandle() );
		crUpdateWindows();
		}

	DestroyModelRenderData ( &ModelCommands );
	return 0;
	}
