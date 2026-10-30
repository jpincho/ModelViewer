#pragma once
#include <stdbool.h>
#include "ResourceLoading/ResourceHub.h"
#include "ResourceLoading/MaterialDescriptor.h"
#include "ResourceLoading/ModelData.h"
#include "Camera.h"

struct ModelRenderOptions
	{
	float Scale;
	};

bool SetModelToRender ( const ResourceHandle NewModelHandle );
void SetModelRenderOptions ( const struct ModelRenderOptions NewOptions );
struct ModelRenderOptions GetModelRenderOptions ( void );

void UpdateRenderingForCamera ( struct CameraData *Camera );
void RenderModel ( void );
void FreeRenderingData ( void );