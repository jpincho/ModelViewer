#pragma once
#include <C_CrossRenderer/CrossRenderer.h>
#include "Camera.h"
#include "ResourceLoading/ModelData.h"
#include "BaseTransform.h"
#include "Animator.h"

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
	struct AnimatorData Animator;
	struct AnimationDescriptor *OriginalAnimationDescriptor;
	mat4 InstanceMatrix;
	};

bool BuildModelRenderData ( struct ModelRenderData *RenderData, const struct ModelData *InputModelData );
void UpdateModelRenderDataWithTransform ( struct ModelRenderData *RenderData, struct BaseTransform *Transform, const float Scale );
void UpdateModelRenderDataWithCamera ( const struct ModelRenderData *RenderData, struct CameraData *Camera );
bool SetModelRenderDataAnimation ( struct ModelRenderData *RenderData, struct AnimationDescriptor *Animation );
void UpdateModelRenderDataWithAnimationTime ( struct ModelRenderData *RenderData, const double Time );
void DestroyModelRenderData ( struct ModelRenderData *RenderData );
void RenderModel ( const struct ModelRenderData *RenderData );

