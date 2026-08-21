#include <Platform/Platform.h>
#include <Platform/PlatformTime.h>
#include <Platform/Logger.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include "EventDispatcher.h"
#include "ModelViewer.h"
#include "Camera.h"
#include "CameraInput.h"
#include "Utils.h"

static crFramebufferHandle MainFramebuffer = NULL;
static CameraData MainCamera = {0};
static bool ShouldQuit = false;
static char *ModelFilename = NULL;
static float ModelScale = 1.0f;
#define TEST_FILENAME "/home/jpincho/Models/glTF-Sample-Models/2.0/AnimatedCube/glTF/AnimatedCube.gltf"
static bool KeyState[256] = {0};
static crRenderCommand *ModelRenderCommands = NULL;
static unsigned ModelRenderCommandCount = 0;
static BaseTransformStruct ModelBaseTransform = {0};
static ResourceHandle ModelResourceHandle = ResourceHandle_Invalid;

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

static void WindowClosed_Callback ( const crWindowHandle Handle )
	{
	ShouldQuit = true;
	}

static void WindowResized_Callback ( const crWindowHandle Handle, const uvec2 NewSize )
	{
	if ( MainFramebuffer != NULL )
		{
		crDeleteFramebuffer ( MainFramebuffer );
		MainFramebuffer = NULL;
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

	MainFramebuffer = crCreateFramebuffer ( FramebufferDescriptor );

	SetCameraPerspectiveParameters ( &MainCamera, MATH_PI_2, ( float ) ClientAreaDimensions.x / ( float ) ClientAreaDimensions.y, 0.1f, 1000.0f );
	UpdateCameraMatrices ( &MainCamera );
	}

static bool LoadModel ( const char *Filename, const char *ResourceName )
	{
	LOG_DEBUG ( "Loading model from '%s'" );

	ResourceHandle Handle = LoadModelFileIntoResourceHub ( Filename, ResourceName );
	if ( Handle == ResourceHandle_Invalid )
		return false;

	LOG_DEBUG ( "Model loaded" );

	ModelData *LoadedModelData = ResourceHub_GetModelData ( Handle );
	if ( LoadedModelData == NULL )
		return false;

	if ( BuildRenderCommandArrayForModelData ( &ModelRenderCommands, &ModelRenderCommandCount, LoadedModelData ) == false )
		return false;

	return true;
	}

int main ( int argc, char *argv[] )
	{
	crRendererConfiguration CrossRendererConfiguration;
	crSetConfigurationToDefault ( &CrossRendererConfiguration );
	CrossRendererConfiguration.InitialWindowDescriptor.Title = "Model Viewer";
	if ( crInitialize ( CrossRendererConfiguration ) == false )
		return -1;

	WindowResized_Callback ( crGetMainWindowHandle(), MAKE_VEC2 ( uvec2, 0, 0 ) );
	if ( !MainFramebuffer )
		return -1;

	crWindowManagerCallbacks Callbacks = { 0 };
	Callbacks.WindowClosed = WindowClosed_Callback;
	Callbacks.WindowResized = WindowResized_Callback;
	Callbacks.KeyStateChanged = KeyStateChanged_Callback;
	crSetWindowManagerCallbacks ( Callbacks );

	InitializeCamera ( &MainCamera );

	math_vec3_set ( &ModelBaseTransform.Position, 0.0f, 0.0f, 0.0f );
	ModelBaseTransform.Orientation = MATH_QUAT_IDENTITY;
	UpdateCachedValues ( &ModelBaseTransform );
	if ( LoadModel ( TEST_FILENAME, "Model" ) == false )
		return -1;

	double LastEndFrameSeconds = Platform_GetPerformanceCurrentTime();
	double TotalDelta = 0.0;
	bool ModelTransformDirty = true, CameraTransformDirty = true;
	bool UpdateRenderCommands = true;
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
		UpdateRenderCommands |= ModelTransformDirty | CameraTransformDirty;
		if ( UpdateRenderCommands )
			{
			mat4 Transform;
			GetTransformationMatrix ( &ModelBaseTransform, &Transform );
			math_mat4_scale_scalar ( &Transform, Transform, ModelScale );
			for ( unsigned Index = 0; Index < ModelRenderCommandCount; ++Index )
				{
				crSetRenderCommandUniformMatrix4Value ( &ModelRenderCommands[Index], crGetShaderUniformHandle ( ModelRenderCommands[Index].Shader, "u_ModelMatrix" ), Transform );
				crSetRenderCommandUniformMatrix4Value ( &ModelRenderCommands[Index], crGetShaderUniformHandle ( ModelRenderCommands[Index].Shader, "u_ViewMatrix" ), MainCamera.CachedValues.ViewMatrix );
				crSetRenderCommandUniformMatrix4Value ( &ModelRenderCommands[Index], crGetShaderUniformHandle ( ModelRenderCommands[Index].Shader, "u_ProjectionMatrix" ), MainCamera.CachedValues.ProjectionMatrix );
				}
			ModelTransformDirty = CameraTransformDirty = false;
			UpdateRenderCommands = false;
			}

		// Actually rendering the frame
		char TempString[256];
		snprintf ( TempString, 256, "%f %f %f", MainCamera.Transform.Position.x, MainCamera.Transform.Position.y, MainCamera.Transform.Position.z );
		crSetWindowTitle ( crGetMainWindowHandle(), TempString );

		crStartRenderToWindow ( crGetMainWindowHandle() );
		crClearFramebufferWithDefaultValues ( MainFramebuffer );
		for ( unsigned Index = 0; Index < ModelRenderCommandCount; ++Index )
			crRunCommand ( ModelRenderCommands[Index] );

		crDisplayFramebuffer ( MainFramebuffer, crGetMainWindowHandle() );
		crDisplayWindow ( crGetMainWindowHandle() );
		crUpdateWindows ();
		}

	return 0;
	}
