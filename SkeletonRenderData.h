#pragma once
#include <C_CrossRenderer/CrossRenderer.h>
#include "ModelRenderData.h"

struct SkeletonRenderData
	{
	const struct ModelRenderData *ModelRenderData;
	crRenderCommand SkeletonCommand;
	crShaderBufferHandle SkeletonPositions;
	};

bool BuildSkeletonRenderData ( struct SkeletonRenderData *RenderData, const struct ModelRenderData *InputModelRenderData );
void UpdateSkeletonRenderDataWithCamera ( struct SkeletonRenderData *RenderData, const struct CameraData *Camera );
void UpdateSkeletonRenderData ( struct SkeletonRenderData *RenderData );
void RenderSkeleton ( const struct SkeletonRenderData *RenderData );
void DestroySkeletonRenderData ( struct SkeletonRenderData *RenderData );