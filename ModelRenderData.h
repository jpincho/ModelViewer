#pragma once
#include <C_CrossRenderer/CrossRenderer.h>
#include "ResourceLoading/ModelData.h"
#include "BaseTransform.h"


struct MeshRenderData
	{
	const struct MeshData *OriginalMeshData;
	crRenderCommand Command;
	crShaderBufferHandle BoneShaderBuffer;
	unsigned BoneCount;
	};

struct ModelRenderData
	{
	struct MeshRenderData *MeshRenderData;
	unsigned MeshRenderDataCount;
	const struct ModelData *OriginalModelData;
	unsigned *NodeIndices;
	unsigned *MeshIndices;
	crShaderBufferHandle BoneShaderBuffer;
	unsigned BoneCount;
	};

bool BuildModelRenderData ( struct ModelRenderData *RenderData, const struct ModelData *InputModelData );
void UpdateModelRenderData ( const struct ModelRenderData *RenderData, struct BaseTransform *Transform, const float Scale, const mat4 ViewMatrix, const mat4 ProjectionMatrix );
void DestroyModelRenderData ( struct ModelRenderData *RenderData );

