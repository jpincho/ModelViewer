#pragma once
#include <C_CrossRenderer/CrossRenderer.h>
#include "ResourceLoading/ModelData.h"
#include "BaseTransform.h"

typedef struct
	{
	crRenderCommand *Commands;
	const ModelData *OriginalModelData;
	crShaderBufferHandle BoneShaderBuffer;
	unsigned BoneCount;
	unsigned *NodeIndices;
	unsigned CommandCount;
	} ModelRenderData;

bool BuildModelRenderData ( ModelRenderData *Output, const ModelData *InputModelData );
void UpdateModelRenderData ( const ModelRenderData *RenderData, BaseTransformStruct *Transform, const float Scale, const mat4 ViewMatrix, const mat4 ProjectionMatrix );
void DestroyModelRenderData ( ModelRenderData *RenderData );
