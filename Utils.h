#pragma once
#include <stdbool.h>
#include "ResourceLoading/MeshData.h"
#include "ResourceLoading/ResourceHub.h"
#include "ShaderCreator/ShaderCreator.h"
#include "Camera.h"

scShaderDefinition BuildShaderDefinitionForMeshData ( const struct MeshData *InputMeshData );
bool BuildRenderCommandForMeshData ( crRenderCommand *Command, const struct MeshData *InputMeshData, const crShaderHandle Shader );
ResourceHandle LoadModelFileIntoResourceHub ( const char *Filename, const char *ResourceName );
void AdjustToFit ( const struct AABB BBox, struct CameraData *Camera, float *Scale );
unsigned RoundUpToNearestMultiple ( const unsigned Number, const unsigned Multiple );