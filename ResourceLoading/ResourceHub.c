#include "ResourceHub.h"
#include <Platform/Logger.h>
#include <Platform/PointerList.h>
#include <string.h>

typedef struct ResourceEntry
	{
	union
		{
		struct
			{
			crTextureHandle TextureHandle;
			crTextureDescriptor Descriptor;
			} TextureResource;
		struct
			{
			MaterialData Data;
			MaterialDescriptor Descriptor;
			} MaterialResource;
		struct
			{
			MeshData Data;
			MeshDescriptor Descriptor;
			} MeshResource;
		struct
			{
			ModelData Data;
			ModelDescriptor Descriptor;
			} ModelResource;
		struct
			{
			crShaderHandle ShaderHandle;
			scShaderDefinition ShaderDefinition;
			crShaderCode ShaderCode;
			bool UseCreator;
			} ShaderResource;
		};
	ResourceHub_Type Type;
	char *Name;
	bool Built;
	} ResourceEntry;

static PointerList Entries;


// Internal functions to abstract from container
static ResourceEntry *GetResourceEntryFromHandle ( const ResourceHandle Handle )
	{
	PointerListNode *Node = ( PointerListNode * ) Handle;
	ResourceEntry *EntryData = ( ResourceEntry* ) PointerList_GetNodeData ( Node );
	return EntryData;
	}

static const char *Stringify_ResourceHub_Type ( const ResourceHub_Type Value )
	{
	switch ( Value )
		{
#define STRINGIFY(X) case ResourceType_##X: return #X;
			STRINGIFY ( Texture );
			STRINGIFY ( Material );
			STRINGIFY ( Mesh );
			STRINGIFY ( Model );
			STRINGIFY ( Shader );
#undef STRINGIFY
		}
	}

ResourceHandle ResourceHub_CreateEntry ( const ResourceHub_Type Type, const char *Name )
	{
	ResourceEntry *NewEntry = calloc ( 1, sizeof ( ResourceEntry ) );
	if ( !NewEntry )
		return NULL;
	NewEntry->Type = Type;
	NewEntry->Name = strdup ( Name );
	LOG_DEBUG ( "Added resource '%s' type '%s'", Name, Stringify_ResourceHub_Type ( Type ) );
	PointerListNode *Node = PointerList_AddAtEnd ( &Entries, NewEntry );
	return Node;
	}

bool ResourceHub_DestroyEntry ( const ResourceHandle EntryHandle )
	{
	PointerListNode *Node = ( PointerListNode * ) EntryHandle;
	ResourceEntry *EntryData = ( ResourceEntry* ) PointerList_GetNodeData ( Node );

	switch ( EntryData->Type )
		{
		case ResourceType_Texture:
			{
			crDestroyTextureDescriptor ( &EntryData->TextureResource.Descriptor );
			if ( EntryData->TextureResource.TextureHandle != crTextureHandle_Invalid )
				crDeleteTexture ( EntryData->TextureResource.TextureHandle );
			break;
			}
		case ResourceType_Material:
			{
			DestroyMaterialDescriptor ( &EntryData->MaterialResource.Descriptor );
			break;
			}
		case ResourceType_Mesh:
			{
			DestroyMeshDescriptor ( &EntryData->MeshResource.Descriptor );
			if ( EntryData->Built == true )
				DestroyMeshData ( &EntryData->MeshResource.Data );
			break;
			}
		case ResourceType_Model:
			{
			break;
			}
		case ResourceType_Shader:
			{
			SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.FragmentShader );
			SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.VertexShader );
			SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.GeometryShader );
			if ( EntryData->ShaderResource.ShaderHandle != crShaderHandle_Invalid )
				crDeleteShader ( EntryData->ShaderResource.ShaderHandle );
			break;
			}
		}

	SAFE_DEL_C ( EntryData->Name );
	free ( EntryData );
	PointerList_DestroyNode ( &Entries, Node );
	return true;
	}

bool ResourceHub_SetTextureDescriptor ( const ResourceHandle EntryHandle, const crTextureDescriptor *Descriptor )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Texture )
		return false;
	crCopyTextureDescriptor ( &EntryData->TextureResource.Descriptor, Descriptor );
	EntryData->Built = false;
	return true;
	}

crTextureDescriptor *ResourceHub_GetTextureDescriptor ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Texture )
		return false;
	return &EntryData->TextureResource.Descriptor;
	}

crTextureHandle ResourceHub_GetTextureHandle ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Texture )
		return NULL;
	if ( ResourceHub_BuildResource ( EntryHandle ) == false )
		return NULL;
	return EntryData->TextureResource.TextureHandle;
	}

bool ResourceHub_SetMaterialDescriptor ( const ResourceHandle EntryHandle, const MaterialDescriptor *Descriptor )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Material )
		return false;
	CopyMaterialDescriptor ( &EntryData->MaterialResource.Descriptor, Descriptor );
	EntryData->Built = false;
	return true;
	}

MaterialDescriptor *ResourceHub_GetMaterialDescriptor ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Material )
		return false;
	return &EntryData->MaterialResource.Descriptor;
	}

MaterialData *ResourceHub_GetMaterialData ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Material )
		return NULL;
	if ( ResourceHub_BuildResource ( EntryHandle ) == false )
		return NULL;
	return &EntryData->MaterialResource.Data;
	}

bool ResourceHub_SetMeshDescriptor ( const ResourceHandle EntryHandle, const MeshDescriptor *Descriptor )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Mesh )
		return false;
	CopyMeshDescriptor ( &EntryData->MeshResource.Descriptor, Descriptor );
	EntryData->Built = false;
	return true;
	}

MeshDescriptor *ResourceHub_GetMeshDescriptor ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Mesh )
		return false;
	return &EntryData->MeshResource.Descriptor;
	}

MeshData *ResourceHub_GetMeshData ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Mesh )
		return NULL;
	if ( ResourceHub_BuildResource ( EntryHandle ) == false )
		return NULL;
	return &EntryData->MeshResource.Data;
	}

bool ResourceHub_SetModelDescriptor ( const ResourceHandle EntryHandle, const ModelDescriptor *Descriptor )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Model )
		return false;
	CopyModelDescriptor ( &EntryData->ModelResource.Descriptor, Descriptor );
	EntryData->Built = false;
	return true;
	}

ModelDescriptor *ResourceHub_GetModelDescriptor ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Model )
		return false;
	return &EntryData->ModelResource.Descriptor;
	}

ModelData *ResourceHub_GetModelData ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Model )
		return NULL;
	if ( ResourceHub_BuildResource ( EntryHandle ) == false )
		return NULL;
	return &EntryData->ModelResource.Data;
	}

bool ResourceHub_SetShaderDefinition ( const ResourceHandle EntryHandle, const scShaderDefinition *Definition )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Shader )
		return false;
	SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.VertexShader );
	SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.FragmentShader );
	SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.GeometryShader );
	EntryData->ShaderResource.ShaderDefinition = *Definition;
	EntryData->ShaderResource.UseCreator = true;
	EntryData->Built = false;
	return true;
	}

bool ResourceHub_SetShaderCode ( const ResourceHandle EntryHandle, const crShaderCode Code )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Shader )
		return false;
	SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.VertexShader );
	SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.FragmentShader );
	SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.GeometryShader );
	EntryData->ShaderResource.ShaderCode.FragmentShader = ( Code.FragmentShader ? strdup ( Code.FragmentShader ) : NULL );
	EntryData->ShaderResource.ShaderCode.GeometryShader = ( Code.GeometryShader ? strdup ( Code.GeometryShader ) : NULL );
	EntryData->ShaderResource.ShaderCode.VertexShader = ( Code.VertexShader ? strdup ( Code.VertexShader ) : NULL );

	EntryData->ShaderResource.UseCreator = false;
	EntryData->Built = false;
	return true;
	}

crShaderCode *ResourceHub_GetShaderCode ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Shader )
		return false;
	return &EntryData->ShaderResource.ShaderCode;
	}

crShaderHandle ResourceHub_GetShaderHandle ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );
	if ( EntryData->Type != ResourceType_Shader )
		return NULL;
	if ( ResourceHub_BuildResource ( EntryHandle ) == false )
		return NULL;
	return EntryData->ShaderResource.ShaderHandle;
	}

ResourceHandle ResourceHub_FindResource ( const ResourceHub_Type Type, const char *Name )
	{
	PointerListNode *Node = PointerList_GetFirst ( &Entries );
	while ( Node != NULL )
		{
		ResourceEntry *EntryData = ( ResourceEntry * ) PointerList_GetNodeData ( Node );
		if ( ( EntryData->Type == Type ) && ( strcmp ( Name, EntryData->Name ) == 0 ) )
			return Node;
		Node = PointerList_GetNextNode ( Node );
		}
	return NULL;
	}

bool ResourceHub_BuildResource ( const ResourceHandle EntryHandle )
	{
	ResourceEntry *EntryData = GetResourceEntryFromHandle ( EntryHandle );

	if ( EntryData->Built == true )
		return true;
	switch ( EntryData->Type )
		{
		case ResourceType_Texture:
			{
			if ( EntryData->TextureResource.TextureHandle != crTextureHandle_Invalid )
				{
				crDeleteTexture ( EntryData->TextureResource.TextureHandle );
				EntryData->TextureResource.TextureHandle = crTextureHandle_Invalid;
				}
			LOG_DEBUG ( "Loading texture '%s'", EntryData->Name );
			EntryData->TextureResource.TextureHandle = crCreateTexture ( EntryData->TextureResource.Descriptor );
			EntryData->Built = true;
			LOG_DEBUG ( "Loaded texture with handle %p", EntryData->TextureResource.TextureHandle );
			return ( EntryData->TextureResource.TextureHandle != crTextureHandle_Invalid );
			}
		case ResourceType_Material:
			{
			LOG_DEBUG ( "Building material '%s'", EntryData->Name );

			// Set all textures for this material
			for ( unsigned TextureIndex = 0; TextureIndex < MatTexType_Count; ++TextureIndex )
				{
				if ( EntryData->MaterialResource.Descriptor.Textures[TextureIndex].TextureName == NULL )
					{
					EntryData->MaterialResource.Data.Textures[TextureIndex].Handle = crTextureHandle_Invalid;
					continue;
					}
				ResourceHandle HubTextureHandle = ResourceHub_FindResource ( ResourceType_Texture, EntryData->MaterialResource.Descriptor.Textures[TextureIndex].TextureName );
				if ( HubTextureHandle == NULL )
					{
					LOG_ERROR ( "Unable to find texture '%s' in resource hub", EntryData->MaterialResource.Descriptor.Textures[TextureIndex].TextureName );
					return false;
					}

				if ( ResourceHub_BuildResource ( HubTextureHandle ) == false )
					return false;

				EntryData->MaterialResource.Data.Textures[TextureIndex].Handle = ResourceHub_GetTextureHandle ( HubTextureHandle );
				EntryData->MaterialResource.Data.Textures[TextureIndex].FilterSettings = EntryData->MaterialResource.Descriptor.Textures[TextureIndex].FilterSettings;
				EntryData->MaterialResource.Data.Textures[TextureIndex].WrapSettings = EntryData->MaterialResource.Descriptor.Textures[TextureIndex].WrapSettings;
				}
			math_vec3_copy ( &EntryData->MaterialResource.Data.Diffuse, EntryData->MaterialResource.Descriptor.Diffuse );
			math_vec3_copy ( &EntryData->MaterialResource.Data.Emissive, EntryData->MaterialResource.Descriptor.Emissive );
			math_vec3_copy ( &EntryData->MaterialResource.Data.Specular, EntryData->MaterialResource.Descriptor.Specular );
			EntryData->MaterialResource.Data.Shininess = EntryData->MaterialResource.Descriptor.Shininess;
			EntryData->Built = true;
			return true;
			}
		case ResourceType_Mesh:
			{
			LOG_DEBUG ( "Building mesh '%s'", EntryData->Name );
			if ( BuildMeshDataFromDescriptor ( EntryData->MeshResource.Descriptor, &EntryData->MeshResource.Data ) == false )
				return false;
			if ( EntryData->MeshResource.Descriptor.MaterialName != NULL )
				{
				ResourceHandle MaterialResourceHandle = ResourceHub_FindResource ( ResourceType_Material, EntryData->MeshResource.Descriptor.MaterialName );
				if ( MaterialResourceHandle == NULL )
					{
					LOG_ERROR ( "Unable to find material '%s' in resource hub", EntryData->MeshResource.Descriptor.MaterialName );
					return false;
					}
				if ( ResourceHub_BuildResource ( MaterialResourceHandle ) == false )
					return false;
				EntryData->MeshResource.Data.Material = *ResourceHub_GetMaterialData ( MaterialResourceHandle );
				}
			EntryData->Built = true;
			return true;
			}
		case ResourceType_Model:
			{
			LOG_DEBUG ( "Building model '%s'", EntryData->Name );
			for ( unsigned Index = 0; Index < EntryData->ModelResource.Descriptor.MeshCount; ++Index )
				{
				ResourceHandle ResourceHandle = ResourceHub_FindResource ( ResourceType_Mesh, EntryData->ModelResource.Descriptor.Meshes[Index].MeshName );
				if ( ResourceHandle == NULL )
					{
					LOG_ERROR ( "Missing mesh '%s'", EntryData->ModelResource.Descriptor.Meshes[Index].MeshName );
					return false;
					}
				if ( ResourceHub_BuildResource ( ResourceHandle ) == false )
					{
					LOG_ERROR ( "Unable to build mesh '%s'", EntryData->ModelResource.Descriptor.Meshes[Index].MeshName );
					return false;
					}
				}

			EntryData->ModelResource.Data.Meshes = calloc ( EntryData->ModelResource.Descriptor.MeshCount, sizeof ( MeshData ) );
			for ( unsigned Index = 0; Index < EntryData->ModelResource.Descriptor.MeshCount; ++Index )
				{
				ResourceHandle ResourceHandle = ResourceHub_FindResource ( ResourceType_Mesh, EntryData->ModelResource.Descriptor.Meshes[Index].MeshName );
				EntryData->ModelResource.Data.Meshes[Index] = *ResourceHub_GetMeshData ( ResourceHandle );
				}
			EntryData->ModelResource.Data.MeshCount = EntryData->ModelResource.Descriptor.MeshCount;

			EntryData->ModelResource.Data.NodeTree = calloc ( EntryData->ModelResource.Descriptor.NodeCount, sizeof ( MeshTreeNode ) );
			for ( unsigned Index = 0; Index < EntryData->ModelResource.Descriptor.NodeCount; ++Index )
				{
				CopyMeshTreeNode ( &EntryData->ModelResource.Data.NodeTree[Index], &EntryData->ModelResource.Descriptor.Nodes[Index] );
				}
			EntryData->ModelResource.Data.NodeCount = EntryData->ModelResource.Descriptor.NodeCount;
			EntryData->Built = true;
			return true;
			}
		case ResourceType_Shader:
			{
			LOG_DEBUG ( "Building shader '%s'", EntryData->Name );
			SAFE_DELETE_SHADER ( EntryData->ShaderResource.ShaderHandle );
			if ( EntryData->ShaderResource.UseCreator )
				{
				SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.VertexShader );
				SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.FragmentShader );
				SAFE_DEL_C ( EntryData->ShaderResource.ShaderCode.GeometryShader );
				EntryData->ShaderResource.ShaderHandle = scAssembleShader ( EntryData->ShaderResource.ShaderDefinition );
				}
			else
				{
				EntryData->ShaderResource.ShaderHandle = crCreateShader ( EntryData->ShaderResource.ShaderCode );
				}
			EntryData->Built = ( EntryData->ShaderResource.ShaderHandle != crShaderHandle_Invalid );
			return ( EntryData->ShaderResource.ShaderHandle != crShaderHandle_Invalid );
			}
		default:
			{
			LOG_ERROR ( "Unhandled resource type" );
			return false;
			}
		}
	return false;
	}
