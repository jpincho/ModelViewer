#include "CameraInput.h"
#include "Camera.h"
#include "EventDispatcher.h"
#include <memory.h>

struct
	{
	CameraData *MyCamera;
	float CameraRotationSpeed, CameraMovementSpeed;
	struct
		{
		bool MoveNegativeX, MoveNegativeY, MoveNegativeZ, MovePositiveX, MovePositiveY, MovePositiveZ;
		bool RotateNegativeX, RotateNegativeY, RotateNegativeZ, RotatePositiveX, RotatePositiveY, RotatePositiveZ;
		} Keys;
	bool MouseViewEnabled;
	ivec2 MouseMovement;
	float MouseSensitivity;
	bool Dirty;
	} CameraInputData;

static void CameraInput_OnEvent ( const edWindowEvent Event )
	{
	switch ( Event.EventType )
		{
		case edWindowEventType_KeyStateChanged:
			{
			switch ( Event.EventData.KeyStateChanged.Key )
				{
				case crKeyCode_W:
					CameraInputData.Keys.MoveNegativeZ = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_S:
					CameraInputData.Keys.MovePositiveZ = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_A:
					CameraInputData.Keys.MoveNegativeX = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_D:
					CameraInputData.Keys.MovePositiveX = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_Z:
					CameraInputData.Keys.MovePositiveY = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_X:
					CameraInputData.Keys.MoveNegativeY = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_Up:
					CameraInputData.Keys.RotatePositiveX = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_Down:
					CameraInputData.Keys.RotateNegativeX = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_Left:
					CameraInputData.Keys.RotatePositiveY = Event.EventData.KeyStateChanged.State;
					break;
				case crKeyCode_Right:
					CameraInputData.Keys.RotateNegativeY = Event.EventData.KeyStateChanged.State;
					break;
				}
			break;
			}
		case edWindowEventType_MouseMoved:
			{
			CameraInputData.MouseMovement.x += Event.EventData.MouseMoved.Delta.x;
			CameraInputData.MouseMovement.y += Event.EventData.MouseMoved.Delta.y;
			break;
			}
		}
	}

void CameraInput_Initialize ( CameraData *NewCameraData )
	{
	memset ( &CameraInputData, 0, sizeof ( CameraInputData ) );
	CameraInputData.MouseSensitivity = 1.0f;
	CameraInputData.MyCamera = NewCameraData;
	CameraInputData.CameraRotationSpeed = MATH_PI / 8;
	CameraInputData.CameraMovementSpeed = 2.0f;
	CameraInputData.MouseViewEnabled = true;
	CameraInputData.Dirty = true;
	edAddEventListener ( CameraInput_OnEvent );
	}

void CameraInput_ProcessCameraMovement ( const float TimeDelta )
	{
	// Camera movement
	vec3 CameraMovementDirection = MATH_VEC3_ZERO;
	if ( CameraInputData.Keys.MoveNegativeX )
		--CameraMovementDirection.x;
	if ( CameraInputData.Keys.MovePositiveX )
		++CameraMovementDirection.x;
	if ( CameraInputData.Keys.MoveNegativeY )
		--CameraMovementDirection.y;
	if ( CameraInputData.Keys.MovePositiveY )
		++CameraMovementDirection.y;
	if ( CameraInputData.Keys.MoveNegativeZ )
		--CameraMovementDirection.z;
	if ( CameraInputData.Keys.MovePositiveZ )
		++CameraMovementDirection.z;

	vec3 CameraRotation = MATH_VEC3_ZERO;
	if ( CameraInputData.Keys.RotateNegativeX )
		--CameraRotation.x;
	if ( CameraInputData.Keys.RotatePositiveX )
		++CameraRotation.x;
	if ( CameraInputData.Keys.RotateNegativeY )
		--CameraRotation.y;
	if ( CameraInputData.Keys.RotatePositiveY )
		++CameraRotation.y;

	BaseTransformStruct CameraTransform = CameraInputData.MyCamera->Transform;
	CameraInputData.Dirty = false;
	if ( math_vec3_is_zero ( CameraMovementDirection ) == false )
		{
		math_vec3_normalize ( &CameraMovementDirection );
		CameraMovementDirection = math_multiply_vec3_by_quat ( CameraMovementDirection, CameraTransform.Orientation );
		math_vec3_normalize ( &CameraMovementDirection );
		math_vec3_scale_inplace ( &CameraMovementDirection, CameraInputData.CameraMovementSpeed * TimeDelta );
		math_vec3_add ( &CameraTransform.Position, CameraTransform.Position, CameraMovementDirection );
		CameraInputData.Dirty = true;
		}
	if ( ( CameraInputData.MouseViewEnabled ) && ( math_ivec2_is_zero ( CameraInputData.MouseMovement ) == false ) )
		{
		CameraRotation.x += -CameraInputData.MouseMovement.y * CameraInputData.MouseSensitivity * CameraInputData.CameraRotationSpeed;
		CameraRotation.y += -CameraInputData.MouseMovement.x * CameraInputData.MouseSensitivity * CameraInputData.CameraRotationSpeed;
		math_ivec2_set_to_zero ( &CameraInputData.MouseMovement );
		}
	// This is for FPS style camera, where Y always points up.
	if ( math_vec3_is_zero ( CameraRotation ) == false )
		{
		static const vec3 WorldXAxis = { 1.0f, 0.0f, 0.0f };
		static const vec3 WorldYAxis = { 0.0f, 1.0f, 0.0f };
		static const vec3 WorldZAxis = { 0.0f, 0.0f, 1.0f };

		vec3 CameraXAxis, CameraYAxis, CameraZAxis;
		GetDirectionVectorsFromBaseTransformStruct ( &CameraTransform, &CameraXAxis, &CameraYAxis, &CameraZAxis );

		quat YRotation, XRotation;
		math_quat_from_axis_angle ( &YRotation, WorldYAxis, CameraRotation.y * TimeDelta );
		math_quat_from_axis_angle ( &XRotation, CameraXAxis, CameraRotation.x * TimeDelta );

		math_quat_normalize ( &XRotation );
		math_quat_normalize ( &YRotation );
		math_quat_multiply_by_quat ( &CameraTransform.Orientation, YRotation, CameraTransform.Orientation );
		math_quat_multiply_by_quat ( &CameraTransform.Orientation, XRotation, CameraTransform.Orientation );

		math_quat_normalize ( &CameraTransform.Orientation );
		CameraInputData.Dirty = true;
		}

	if ( CameraInputData.Dirty == true )
		{
		CameraInputData.MyCamera->Transform = CameraTransform;
		CameraInputData.MyCamera->Transform.CachedValues.Dirty = true;
		}
	}

void CameraInput_SetMouseViewEnabled ( const bool Enabled )
	{
	CameraInputData.MouseViewEnabled = Enabled;
	}

bool CameraInput_GetMouseViewEnabled ( void )
	{
	return CameraInputData.MouseViewEnabled;
	}

bool CameraInput_HasMovedThisFrame ( void )
	{
	return CameraInputData.Dirty;
	}