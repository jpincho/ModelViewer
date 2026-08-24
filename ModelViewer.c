#include <Platform/Platform.h>
#include <Platform/PlatformTime.h>
#include <Platform/Logger.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include "EventDispatcher.h"
#include "ModelViewer.h"
#include "Camera.h"
#include "CameraInput.h"
#include "Utils.h"
#include "ResourceLoading/ResourceHub.h"

static crFramebufferHandle MainWindowFramebuffer = NULL;
static CameraData MainCamera = {0};
static bool ShouldQuit = false;
static char *ModelFilename = NULL;
static float ModelScale = 1.0f;
static bool ForceRenderCommandUpdate = false;
static ivec2 LastMousePosition;
static bool HaveLastMousePosition = false;

//#define TEST_FILENAME "/home/jpincho/Models/glTF-Sample-Models/2.0/AnimatedCube/glTF/AnimatedCube.gltf"
#define TEST_FILENAME "/home/jpincho/Models/glTF-Sample-Models/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf"

static void KeyStateChanged_Callback ( const crWindowHandle Handle, const crKeyCode KeyCode, const bool State )
	{
	edSendKeyStateChangedEvent ( Handle, KeyCode, State );
	switch ( KeyCode )
		{
		case crKeyCode_Escape:
			{
			ShouldQuit = true;
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
	SetCameraPosition ( Camera, ( vec3 ) { 0.0f, 0.0f, 4.0f } );
	UpdateCameraMatrices ( Camera );

	return true;
	}

static ResourceHandle LoadModel ( const char *Filename, const char *ResourceName )
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
		return false;
		}
	AABB_InitEmpty ( &Descriptor->AABBox );
	for ( unsigned MeshIndex = 0; MeshIndex < Descriptor->MeshCount; ++MeshIndex )
		{
		AABB_IncludePoint ( &Descriptor->AABBox, Descriptor->Meshes[MeshIndex].BoundingBox.Min );
		AABB_IncludePoint ( &Descriptor->AABBox, Descriptor->Meshes[MeshIndex].BoundingBox.Max );
		}
	LOG_DEBUG ( "Model loaded" );
	return Handle;
	}

int main ( int argc, char *argv[] )
	{
	BaseTransformStruct ModelBaseTransform = {0};
	ModelRenderData ModelCommands = {0};
	ModelData *LoadedModelData = NULL;
	ResourceHandle LoadedModelHandle = ResourceHandle_Invalid;

	if ( InitializeRenderer() == false )
		return -1;

	if ( InitializeCamera ( &MainCamera ) == false )
		return -1;

	math_vec3_set ( &ModelBaseTransform.Position, 0.0f, 0.0f, 0.0f );
	ModelBaseTransform.Orientation = MATH_QUAT_IDENTITY;
	UpdateCachedValues ( &ModelBaseTransform );
	LoadedModelHandle = LoadModel ( TEST_FILENAME, "Model" );
	if ( LoadedModelHandle == ResourceHandle_Invalid )
		return -1;
	LoadedModelData = ResourceHub_GetModelData ( LoadedModelHandle );

	if ( BuildModelRenderData ( &ModelCommands, LoadedModelData ) == false )
		return -1;
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
		if ( ModelTransformDirty || CameraTransformDirty || ForceRenderCommandUpdate )
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
		crUpdateWindows ();
		}

	DestroyModelRenderData ( &ModelCommands );
	return 0;
	}
