#pragma once
#include <Platform/Platform.h>
#include "BaseTransform.h"

BEGIN_C_DECLARATIONS
typedef struct
	{
	struct
		{
		float FOV, AspectRatio, Near, Far;
		} PerspectiveParameters;
	BaseTransformStruct Transform;

	struct
		{
		mat4 ProjectionMatrix, ViewMatrix, ViewProjectionMatrix;
		} CachedValues;
	} CameraData;

void SetCameraPosition ( CameraData *Camera, const vec3 NewPosition );
void SetCameraOrientation ( CameraData *Camera, const quat NewOrientation );
void SetCameraPerspectiveParameters ( CameraData *Camera, const float NewFOV, const float NewAspectRatio, const float NewNear, const float NewFar );
void UpdateCameraMatrices ( CameraData *CameraData );

END_C_DECLARATIONS
