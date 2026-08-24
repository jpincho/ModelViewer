#pragma once
#include <C_CrossRenderer/CrossRenderer.h>
#include "MaterialDescriptor.h"
#include "MaterialData.h"
#include "MeshDescriptor.h"
#include "MeshData.h"
#include "ModelDescriptor.h"
#include "ModelData.h"
#include "../ShaderCreator/ShaderCreator.h"
#include <stdbool.h>

typedef enum
	{
	ResourceType_Texture,
	ResourceType_Material,
	ResourceType_Mesh,
	ResourceType_Model,
	ResourceType_Shader,
	} ResourceHub_Type;

typedef void *ResourceHandle;
#define ResourceHandle_Invalid NULL

ResourceHandle ResourceHub_CreateEntry ( const ResourceHub_Type Type, const char *Name );
bool ResourceHub_DestroyEntry ( const ResourceHandle EntryHandle );
bool ResourceHub_SetTextureDescriptor ( const ResourceHandle EntryHandle, const crTextureDescriptor *Descriptor );
crTextureDescriptor *ResourceHub_GetTextureDescriptor ( const ResourceHandle EntryHandle );
crTextureHandle ResourceHub_GetTextureHandle ( const ResourceHandle EntryHandle );
bool ResourceHub_SetMaterialDescriptor ( const ResourceHandle EntryHandle, const MaterialDescriptor *Descriptor );
MaterialDescriptor *ResourceHub_GetMaterialDescriptor ( const ResourceHandle EntryHandle );
MaterialData *ResourceHub_GetMaterialData ( const ResourceHandle EntryHandle );
bool ResourceHub_SetMeshDescriptor ( const ResourceHandle EntryHandle, const MeshDescriptor *Descriptor );
MeshDescriptor *ResourceHub_GetMeshDescriptor ( const ResourceHandle EntryHandle );
MeshData *ResourceHub_GetMeshData ( const ResourceHandle EntryHandle );
bool ResourceHub_SetModelDescriptor ( const ResourceHandle EntryHandle, const ModelDescriptor *Descriptor );
ModelDescriptor *ResourceHub_GetModelDescriptor ( const ResourceHandle EntryHandle );
ModelData *ResourceHub_GetModelData ( const ResourceHandle EntryHandle );
bool ResourceHub_SetShaderDefinition ( const ResourceHandle EntryHandle, const scShaderDefinition *Definition );
bool ResourceHub_SetShaderCode ( const ResourceHandle EntryHandle, const crShaderCode Code );
crShaderCode *ResourceHub_GetShaderCode ( const ResourceHandle EntryHandle );
crShaderHandle ResourceHub_GetShaderHandle ( const ResourceHandle EntryHandle );

ResourceHandle ResourceHub_FindResource ( const ResourceHub_Type Type, const char *Name );
bool ResourceHub_BuildResource ( const ResourceHandle EntryHandle );
