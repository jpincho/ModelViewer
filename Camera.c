#include "Camera.h"
void SetCameraPosition ( CameraData *Camera, const vec3 NewPosition )
	{
	Camera->Transform.Position = NewPosition;
	Camera->Transform.CachedValues.Dirty = true;
	}

void SetCameraOrientation ( CameraData *Camera, const quat NewOrientation )
	{
	Camera->Transform.Orientation = NewOrientation;
	Camera->Transform.CachedValues.Dirty = true;
	}

void SetCameraPerspectiveParameters ( CameraData *Camera, const float NewFOV, const float NewAspectRatio, const float NewNear, const float NewFar )
	{
	Camera->PerspectiveParameters.AspectRatio = NewAspectRatio;
	Camera->PerspectiveParameters.Far = NewFar;
	Camera->PerspectiveParameters.Near = NewNear;
	Camera->PerspectiveParameters.FOV = NewFOV;
	math_mat4_set_perspective_matrix ( &Camera->CachedValues.ProjectionMatrix, Camera->PerspectiveParameters.FOV, Camera->PerspectiveParameters.AspectRatio, Camera->PerspectiveParameters.Near, Camera->PerspectiveParameters.Far );
	math_mat4_multiply ( &Camera->CachedValues.ViewProjectionMatrix, Camera->CachedValues.ProjectionMatrix, Camera->CachedValues.ViewMatrix );
	}

void UpdateCameraMatrices ( CameraData *Camera )
	{
	if ( Camera->Transform.CachedValues.Dirty == false )
		return;

	// Update transform
	UpdateCachedValues ( &Camera->Transform );

	vec3 NegZAxis;
	math_vec3_scale ( &NegZAxis, Camera->Transform.CachedValues.ZAxis, -1.0f );
	math_vec3_add ( &NegZAxis, NegZAxis, Camera->Transform.Position );
	math_mat4_set_right_handed_view_matrix ( &Camera->CachedValues.ViewMatrix, Camera->Transform.Position, NegZAxis, Camera->Transform.CachedValues.YAxis );
	math_mat4_multiply ( &Camera->CachedValues.ViewProjectionMatrix, Camera->CachedValues.ProjectionMatrix, Camera->CachedValues.ViewMatrix );
	}
