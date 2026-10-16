#pragma once
#include <stdbool.h>
#include "BaseTransform.h"
#include "Camera.h"
#include "ResourceLoading/MeshData.h"
#include "ResourceLoading/ModelData.h"
#include "ResourceLoading/ResourceHub.h"
#include "ShaderCreator/ShaderCreator.h"
#include <C_CrossRenderer/CrossRenderer.h>

typedef struct
	{
	crRenderCommand *Commands;
	const ModelData *OriginalModelData;
	unsigned *NodeIndices;
	unsigned Count;
	} ModelRenderData;

scShaderDefinition BuildShaderDefinitionForMeshData ( const MeshData *InputMeshData );
bool BuildRenderCommandForMeshData ( crRenderCommand *Command, const MeshData *InputMeshData, const crShaderHandle Shader );
bool BuildModelRenderData ( ModelRenderData *Output, const ModelData *InputModelData );
void UpdateModelRenderData ( const ModelRenderData *RenderData, BaseTransformStruct *Transform, const float Scale, const mat4 ViewMatrix, const mat4 ProjectionMatrix );
void DestroyModelRenderData ( ModelRenderData *RenderData );
ResourceHandle LoadModelFileIntoResourceHub ( const char *Filename, const char *ResourceName );
void ComputeMeshTreeNodeWorldMatrices ( const MeshTreeNode *NodeTree, const unsigned NodeCount, mat4 *WorldMatrices );
float CalculateScaleToFit ( const AABB box, const float CameraDistance, const float VerticalFOV, const float AspectRatio );
float CalculateScaleToFit2 ( const AABB BBox, const float CameraDistance, const float VerticalFOV, const float AspectRatio );

void AdjustToFit ( const AABB BBox, CameraData *Camera, float *Scale );