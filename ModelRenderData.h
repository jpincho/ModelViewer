#pragma once
#include <C_CrossRenderer/CrossRenderer.h>
#include "ResourceLoading/ModelData.h"
#include "BaseTransform.h"

struct ModelRenderData
	{
	crRenderCommand *Commands;
	const struct ModelData *OriginalModelData;
	crShaderBufferHandle BoneShaderBuffer;
	unsigned BoneCount;
	unsigned *NodeIndices;
	unsigned *MeshIndices;
	unsigned CommandCount;
	};

bool BuildModelRenderData ( struct ModelRenderData *RenderData, const struct ModelData *InputModelData );
void UpdateModelRenderData ( const struct ModelRenderData *RenderData, struct BaseTransform *Transform, const float Scale, const mat4 ViewMatrix, const mat4 ProjectionMatrix );
void DestroyModelRenderData ( struct ModelRenderData *RenderData );
