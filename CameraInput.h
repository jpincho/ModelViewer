#pragma once
#include "Camera.h"

void CameraInput_Initialize ( CameraData *NewCamera );
void CameraInput_ProcessCameraMovement ( const float TimeDelta );
void CameraInput_SetMouseViewEnabled ( const bool Enabled );
bool CameraInput_GetMouseViewEnabled ( void );
bool CameraInput_HasMovedThisFrame ( void );