#pragma once
#include <Platform/Platform.h>
#include "BaseTransform.h"

BEGIN_C_DECLARATIONS
struct CameraData
	{
	struct
		{
		float FOV, AspectRatio, Near, Far;
		} PerspectiveParameters;
	struct BaseTransform Transform;

	struct
		{
		mat4 ProjectionMatrix, ViewMatrix, ViewProjectionMatrix;
		} CachedValues;
	};

void SetCameraPosition ( struct CameraData *Camera, const vec3 NewPosition );
void SetCameraOrientation ( struct CameraData *Camera, const quat NewOrientation );
void SetCameraPerspectiveParameters ( struct CameraData *Camera, const float NewFOV, const float NewAspectRatio, const float NewNear, const float NewFar );
void UpdateCameraMatrices ( struct CameraData *CameraData );

END_C_DECLARATIONS
