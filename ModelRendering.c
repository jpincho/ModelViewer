#include "ModelRendering.h"
#include "ModelRenderData.h"
#include <Platform/Logger.h>

static struct
	{
	struct ModelRenderOptions Options;
	struct ModelRenderData ModelRenderData;
	struct BaseTransform BaseTransform;
	ResourceHandle ModelHandle, AnimationHandle;
	} RenderingData = {0};

bool SetModelToRender ( const ResourceHandle NewModelHandle )
	{
	FreeRenderingData();
	if ( NewModelHandle == ResourceHandle_Invalid )
		return true;
	struct ModelData *NewModelData = ResourceHub_GetModelData ( NewModelHandle );
	if ( NewModelData == NULL )
		return false;
	if ( BuildModelRenderData ( &RenderingData.ModelRenderData, NewModelData ) == false )
		return false;
	RenderingData.ModelHandle = NewModelHandle;

	return true;
	}

void SetModelRenderOptions ( const struct ModelRenderOptions NewOptions )
	{
	RenderingData.Options = NewOptions;
	}

struct ModelRenderOptions GetModelRenderOptions ( void )
	{
	return RenderingData.Options;
	}

void UpdateRenderingForCamera ( struct CameraData *Camera )
	{
	UpdateModelRenderData ( &RenderingData.ModelRenderData, &RenderingData.BaseTransform, RenderingData.Options.Scale, Camera->CachedValues.ViewMatrix, Camera->CachedValues.ProjectionMatrix );
	}

void RenderModel ( void )
	{
	for ( unsigned Index = 0; Index < RenderingData.ModelRenderData.MeshRenderDataCount; ++Index )
		{
		crRunCommand ( RenderingData.ModelRenderData.MeshRenderData[Index].Command );
		}
	}


void FreeRenderingData ( void )
	{
	DestroyModelRenderData ( &RenderingData.ModelRenderData );
	math_vec3_set ( &RenderingData.BaseTransform.Position, 0.0f, 0.0f, 0.0f );
	RenderingData.BaseTransform.Orientation = MATH_QUAT_IDENTITY;
	UpdateCachedValues ( &RenderingData.BaseTransform );
	RenderingData.ModelHandle = ResourceHandle_Invalid;
	}
