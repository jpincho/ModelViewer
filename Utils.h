#pragma once
#include <stdbool.h>
#include "ResourceLoading/MeshData.h"
#include "ResourceLoading/ResourceHub.h"
#include "ShaderCreator/ShaderCreator.h"
#include "Camera.h"

scShaderDefinition BuildShaderDefinitionForMeshData ( const MeshData *InputMeshData );
bool BuildRenderCommandForMeshData ( crRenderCommand *Command, const MeshData *InputMeshData, const crShaderHandle Shader );
void ComputeMeshTreeNodeWorldMatrices ( const MeshTreeNode *NodeTree, const unsigned NodeCount, mat4 *WorldMatrices );
ResourceHandle LoadModelFileIntoResourceHub ( const char *Filename, const char *ResourceName );
void AdjustToFit ( const AABB BBox, CameraData *Camera, float *Scale );
unsigned RoundUpToNearestMultiple ( const unsigned Number, const unsigned Multiple );