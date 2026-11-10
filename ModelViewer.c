#include <Platform/Platform.h>
#include <Platform/PlatformTime.h>
#include <Platform/Logger.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include <CrossUI/CrossUI.h>

#include "ResourceLoading/ResourceHub.h"
#include "Camera.h"
#include "EventDispatcher.h"
#include "CameraInput.h"
#include "Utils.h"
#include "ModelRendering.h"

static struct
	{
	crFramebufferHandle MainWindowFramebuffer;
	struct CameraData MainCamera;
	bool ShouldQuit;
	bool ForceRenderCommandUpdate;
	ivec2 LastMousePosition;
	bool HaveLastMousePosition;
	ResourceHandle CurrentAnimationHandle;
	} ModelViewerData = {0};

//#define TEST_FILENAME MODEL_PATH "/GLTF/Character1/untitled.gltf"
#define TEST_FILENAME MODEL_PATH "/GLTF/buster_drone/busterDrone.gltf"

static ResourceHandle LoadModel ( const char *Filename, const bool AutoFit );
static void ResetCamera ( struct CameraData *Camera );

static void KeyStateChanged_Callback ( const crWindowHandle Handle, const crKeyCode KeyCode, const bool State )
	{
	static bool LeftCtrlPressed = false, RightCtrlPressed = false;
	static bool OPressed = false, F1Pressed = false;

	edSendKeyStateChangedEvent ( Handle, KeyCode, State );
	switch ( KeyCode )
		{
		case crKeyCode_Escape:
			{
			ModelViewerData.ShouldQuit = true;
			break;
			}
		case crKeyCode_F1:
			{
			if ( ( State == true ) && ( F1Pressed == false ) )
				{
				struct ModelRenderOptions Options = GetModelRenderOptions();
				Options.Wireframe = !Options.Wireframe;
				SetModelRenderOptions ( Options );
				}
			F1Pressed = State;
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
				SelectedFilename = cuiOpenFileDialog ( NULL, "Load model file", "All valid|*.gltf;*.glb;|GLTF|*.gltf;|GLTF Binary|*.glb" );
				if ( SelectedFilename == NULL )
					{
					LOG_ERROR ( "Error selecting file" );
					break;
					}

				if ( LoadModel ( SelectedFilename, true ) == ResourceHandle_Invalid )
					{
					LOG_ERROR ( "Unable to load model" );
					break;
					}
				}
			OPressed = State;
			break;
			}
		case crKeyCode_R:
			{
			ResetCamera ( &ModelViewerData.MainCamera );
			break;
			}
		}
	}

static void WindowClosed_Callback ( const crWindowHandle Handle )
	{
	ModelViewerData.ShouldQuit = true;
	}

static void MouseMoved_Callback ( const crWindowHandle Handle, const ivec2 NewPosition )
	{
	ivec2 Delta = { 0, 0 };
	if ( ModelViewerData.HaveLastMousePosition )
		{
		Delta.x = NewPosition.x - ModelViewerData.LastMousePosition.x;
		Delta.y = NewPosition.y - ModelViewerData.LastMousePosition.y;
		}
	ModelViewerData.LastMousePosition = NewPosition;
	ModelViewerData.HaveLastMousePosition = true;
	edSendMouseMovedEvent ( Handle, NewPosition, Delta );
	}

static void WindowResized_Callback ( const crWindowHandle Handle, const uvec2 NewSize )
	{
	if ( ModelViewerData.MainWindowFramebuffer != NULL )
		{
		crDeleteFramebuffer ( ModelViewerData.MainWindowFramebuffer );
		ModelViewerData.MainWindowFramebuffer = NULL;
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
	math_vec4_set ( &FramebufferDescriptor.ClearColor, 0.0f, 0.0f, 0.0f, 1.0f );

	ModelViewerData.MainWindowFramebuffer = crCreateFramebuffer ( FramebufferDescriptor );

	SetCameraPerspectiveParameters ( &ModelViewerData.MainCamera, MATH_PI_2, ( float ) ClientAreaDimensions.x / ( float ) ClientAreaDimensions.y, 0.1f, 1000.0f );
	UpdateCameraMatrices ( &ModelViewerData.MainCamera );
	ModelViewerData.ForceRenderCommandUpdate = true;
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
	math_vec4_set ( &FramebufferDescriptor.ClearColor, 0.0f, 0.0f, 0.0f, 1.0f );

	ModelViewerData.MainWindowFramebuffer = crCreateFramebuffer ( FramebufferDescriptor );
	if ( !ModelViewerData.MainWindowFramebuffer )
		return false;
	return true;
	}

static bool InitializeCamera ( struct CameraData *Camera )
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

static ResourceHandle LoadModel ( const char *Filename, const bool AutoFit )
	{
	LOG_DEBUG ( "Loading model from '%s'", Filename );
	ResourceHub_DestroyAll();
	FreeRenderingData();

	ResourceHandle Handle = LoadModelFileIntoResourceHub ( Filename, "Model" );
	if ( Handle == ResourceHandle_Invalid )
		{
		LOG_ERROR ( "Unable to load model" );
		return ResourceHandle_Invalid;
		}
	LOG_DEBUG ( "Model loaded" );
	if ( SetModelToRender ( Handle ) == false )
		{
		ResourceHub_DestroyAll();
		FreeRenderingData();
		return ResourceHandle_Invalid;
		}

	ModelViewerData.CurrentAnimationHandle = ResourceHub_GetResourceByTypeAndIndex ( ResourceType_Animation, 0 );
	if ( ModelViewerData.CurrentAnimationHandle != ResourceHandle_Invalid )
		{
		if ( SetAnimationToRender ( ModelViewerData.CurrentAnimationHandle ) == false )
			{
			LOG_ERROR ( "Animation is not compatible with this model" );
			ModelViewerData.CurrentAnimationHandle = ResourceHandle_Invalid;
			}
		}

	if ( AutoFit )
		{
		float Scale = 1.0f;
		const struct ModelDescriptor *ModelDescriptor;
		ModelDescriptor = ResourceHub_GetModelDescriptor ( Handle );
		if ( ModelDescriptor != NULL )
			{
			AdjustToFit ( ModelDescriptor->AABBox, &ModelViewerData.MainCamera, &Scale );
			struct ModelRenderOptions Options = GetModelRenderOptions();
			Options.Scale = Scale;
			SetModelRenderOptions ( Options );
			}
		}

	UpdateRenderingForCamera ( &ModelViewerData.MainCamera );
	return Handle;
	}

static void ResetCamera ( struct CameraData *Camera )
	{
	uvec2 WindowSize;
	crGetWindowClientAreaDimensions ( crGetMainWindowHandle(), &WindowSize );

	SetCameraPerspectiveParameters ( Camera, MATH_PI_2, ( float ) WindowSize.x / ( float ) WindowSize.y, 0.1f, 1000.0f );
	SetCameraOrientation ( Camera, MATH_QUAT_IDENTITY );
	SetCameraPosition ( Camera, ( vec3 )
		{
		0.0f, 2.5f, 5.0f
		} );
	}

int main ( int argc, char *argv[] )
	{
	if ( InitializeRenderer() == false )
		return -1;

	if ( InitializeCamera ( &ModelViewerData.MainCamera ) == false )
		return -1;
	ResetCamera ( &ModelViewerData.MainCamera );

	if ( cuiInitializeWithArgs ( argc, argv ) == false )
		return -1;

	if ( LoadModel ( TEST_FILENAME, true ) == ResourceHandle_Invalid )
		return -1;

	double LastEndFrameSeconds = Platform_GetPerformanceCurrentTime();
	double TotalDelta = 0.0;
	while ( ModelViewerData.ShouldQuit == false )
		{
		// World tick
		double Seconds = Platform_GetPerformanceCurrentTime();
		double TimeDelta = Seconds - LastEndFrameSeconds;
		if ( TimeDelta == 0.0 )
			TimeDelta = 0.01;
		LastEndFrameSeconds = Seconds;
		TotalDelta += TimeDelta;

		CameraInput_ProcessCameraMovement ( TimeDelta );

		if ( ModelViewerData.MainCamera.Transform.CachedValues.Dirty )
			{
			UpdateCameraMatrices ( &ModelViewerData.MainCamera );
			ModelViewerData.ForceRenderCommandUpdate = true;
			}

		if ( ModelViewerData.CurrentAnimationHandle != ResourceHandle_Invalid )
			UpdateRenderingWithAnimationTime ( TotalDelta );

		// Updating rendering data, if needed
		if ( ModelViewerData.ForceRenderCommandUpdate )
			{
			UpdateRenderingForCamera ( &ModelViewerData.MainCamera );
			ModelViewerData.ForceRenderCommandUpdate = false;
			}

		// Actually rendering the frame
		char TempString[256];
		snprintf ( TempString, 256, "%f %f %f", ModelViewerData.MainCamera.Transform.Position.x, ModelViewerData.MainCamera.Transform.Position.y, ModelViewerData.MainCamera.Transform.Position.z );
		crSetWindowTitle ( crGetMainWindowHandle(), TempString );

		crStartRenderToWindow ( crGetMainWindowHandle() );
		crClearFramebufferWithDefaultValues ( ModelViewerData.MainWindowFramebuffer );
		RenderModel();

		crDisplayFramebuffer ( ModelViewerData.MainWindowFramebuffer, crGetMainWindowHandle() );
		crDisplayWindow ( crGetMainWindowHandle() );
		crUpdateWindows();
		}

	FreeRenderingData();
	return 0;
	}
