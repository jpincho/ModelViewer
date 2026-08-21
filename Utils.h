#pragma once
#include <stdbool.h>
#include "ResourceLoading/MeshData.h"
#include "ResourceLoading/Model.h"
#include "ResourceLoading/ResourceHub.h"
#include "ShaderCreator/ShaderCreator.h"
#include <C_CrossRenderer/CrossRenderer.h>

scShaderDefinition BuildShaderDefinitionForMeshData ( const MeshData *InputMeshData );
bool BuildRenderCommandForMeshData ( crRenderCommand *Command, const MeshData *InputMeshData, const crShaderHandle Shader );
bool BuildRenderCommandArrayForModelData ( crRenderCommand **CommandArray, unsigned *CommandCount, const ModelData *InputModelData );
ResourceHandle LoadModelFileIntoResourceHub ( const char *Filename, const char *ResourceName );