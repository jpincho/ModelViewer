#pragma once
#include <stdbool.h>
#include "ResourceLoading/ResourceHub.h"
#include "ResourceLoading/MaterialDescriptor.h"
#include "ResourceLoading/ModelData.h"
#include "Camera.h"
#include "Animator.h"

struct ModelRenderOptions
	{
	float Scale;
	bool Wireframe;
	};

bool SetModelToRender ( const ResourceHandle NewModelHandle );
void SetModelRenderOptions ( const struct ModelRenderOptions NewOptions );
struct ModelRenderOptions GetModelRenderOptions ( void );

void UpdateRenderingWithAnimationTime ( const double Time );
void UpdateRenderingForCamera ( struct CameraData *Camera );
void RenderModel ( void );
void FreeRenderingData ( void );

bool SetAnimationToRender ( const ResourceHandle NewAnimationHandle );
AnimatorData *GetAnimatorForRendering ( void );
