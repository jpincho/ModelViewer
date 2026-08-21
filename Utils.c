#include "Utils.h"
#include "ResourceLoading/ThreadedFileLoader.h"
#include <string.h>
#include <Platform/Logger.h>

scShaderDefinition BuildShaderDefinitionForMeshData ( const MeshData *InputMeshData )
	{
	scShaderDefinition ShaderDefinition = { 0 };

	// Where to get diffuse component from
	if ( ( InputMeshData->Material.Textures[MatTexType_Diffuse].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.DiffuseSource = scSource_Texture;
	else if ( InputMeshData->Color != crShaderHandle_Invalid )
		ShaderDefinition.DiffuseSource = scSource_Attribute;
	else
		ShaderDefinition.DiffuseSource = scSource_Uniform;

	// Where to get specular component from
	if ( ( InputMeshData->Material.Textures[MatTexType_Specular].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.SpecularSource = scSource_Texture;
	else if ( InputMeshData->Color != crShaderHandle_Invalid )
		ShaderDefinition.SpecularSource = scSource_Attribute;
	else
		ShaderDefinition.SpecularSource = scSource_Uniform;

	// Where to get specular shininess factor from
	if ( ( InputMeshData->Material.Textures[MatTexType_Shininess].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.ShininessSource = scSource_Texture;
	else if ( InputMeshData->Color != crShaderHandle_Invalid )
		ShaderDefinition.ShininessSource = scSource_Attribute;
	else
		ShaderDefinition.ShininessSource = scSource_Uniform;

	// Where to get emissiveness component from
	if ( ( InputMeshData->Material.Textures[MatTexType_Emissive].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.EmissivenessSource = scSource_Texture;
	else if ( InputMeshData->Color != crShaderHandle_Invalid )
		ShaderDefinition.EmissivenessSource = scSource_Attribute;
	else
		ShaderDefinition.EmissivenessSource = scSource_Uniform;

	// Where to get normal vector from
	if ( ( InputMeshData->Material.Textures[MatTexType_Normal].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) ) // && (InputMeshData->BitangentStream.BufferHandle!=crShaderBufferHandle_Invalid) && (InputMeshData->TangentStream.BufferHandle != crShaderBufferHandle_Invalid))
		ShaderDefinition.NormalSource = scSource_Texture;
	else if ( InputMeshData->Normal != crShaderHandle_Invalid )
		ShaderDefinition.NormalSource = scSource_Attribute;
	else
		ShaderDefinition.NormalSource = scSource_Off;

	// What lighting algorithm to use
	if ( ShaderDefinition.NormalSource != scSource_Off )
		ShaderDefinition.LightModel = scLightModel_BlinnPhong;
	else
		ShaderDefinition.LightModel = scLightModel_Off;

	// Whether or not to use skinning
	if ( ( InputMeshData->BoneCount != 0 ) && ( InputMeshData->BoneIndexStream.BufferHandle != crShaderBufferHandle_Invalid ) && ( InputMeshData->BoneWeightStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		{
		ShaderDefinition.Skinning = true;
		ShaderDefinition.BoneCount = InputMeshData->BoneCount;
		}
	return ShaderDefinition;
	}

bool BuildRenderCommandForMeshData ( crRenderCommand *Command, const MeshData *InputMeshData, const crShaderHandle Shader )
	{
	Command->IndexBufferStream = InputMeshData->IndexStream;
	Command->InstanceCount = 1;
	Command->Primitive = crTriangleList;
	crSetRenderCommandShader ( Command, Shader );
	crSetRenderCommandUniformFloat3Value ( Command, crGetShaderUniformHandle ( Shader, "MaterialDiffuse" ), InputMeshData->Material.Diffuse );
	crSetRenderCommandUniformFloat3Value ( Command, crGetShaderUniformHandle ( Shader, "MaterialEmisive" ), InputMeshData->Material.Emissive );
	crSetRenderCommandUniformFloatValue ( Command, crGetShaderUniformHandle ( Shader, "MaterialShininess" ), InputMeshData->Material.Shininess );
	crSetRenderCommandUniformFloat3Value ( Command, crGetShaderUniformHandle ( Shader, "MaterialSpecular" ), InputMeshData->Material.Specular );

	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Position ) ), InputMeshData->PositionStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_TextureCoordinate ) ), InputMeshData->TexCoordStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Normal ) ), InputMeshData->NormalStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Tangent ) ), InputMeshData->TangentStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Bitangent ) ), InputMeshData->BitangentStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Color ) ), InputMeshData->ColorStream );

	crSetRenderCommandIndexShaderBufferBinding ( Command, InputMeshData->IndexStream );

	for ( unsigned TypeIterator = 0; TypeIterator < MatTexType_Count; ++TypeIterator )
		{
		if ( InputMeshData->Material.Textures[TypeIterator].Handle )
			{
			crSetRenderCommandTextureBinding ( Command, crGetShaderUniformHandle ( Shader, scGetStandardTextureUniformName ( TypeIterator ) ), InputMeshData->Material.Textures[TypeIterator] );
			}
		}

	Command->Primitive = crTriangleList;
	Command->StartVertex = 0;
	Command->VertexCount = InputMeshData->VertexCount;
	Command->InstanceCount = 1;
	crSetRenderStateToDefault ( &Command->State );
	Command->State.Culling.Enabled = true;
	Command->State.DepthTest.Enabled = true;
	return true;
	}

bool BuildRenderCommandArrayForModelData ( crRenderCommand **CommandArray, unsigned *CommandCount, const ModelData *InputModelData )
	{
	// Count how many render commands this will take, to pre-allocate memory
	unsigned TotalMeshCount = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < InputModelData->NodeCount; ++NodeIndex )
		{
		TotalMeshCount += InputModelData->NodeTree[NodeIndex].MeshIndexCount;
		}
	if ( TotalMeshCount == 0 )
		return false;
	*CommandCount = TotalMeshCount;
	*CommandArray = calloc ( TotalMeshCount, sizeof ( crRenderCommand ) );
	if ( *CommandArray == NULL )
		return false;
	unsigned *MeshIndices = alloca ( TotalMeshCount * sizeof ( unsigned ) );
	if ( MeshIndices == NULL )
		{
		free ( *CommandArray );
		return false;
		}
	unsigned CurrentPosition = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < InputModelData->NodeCount; ++NodeIndex )
		{
		for ( unsigned MeshIndex = 0; MeshIndex < InputModelData->NodeTree[NodeIndex].MeshIndexCount; ++MeshIndex )
			{
			MeshIndices[CurrentPosition] = InputModelData->NodeTree[NodeIndex].MeshIndices[MeshIndex];
			++CurrentPosition;
			}
		}

	for ( unsigned Index = 0; Index < *CommandCount; ++Index )
		{
		// Load shader for this mesh
		scShaderDefinition ShaderDefs = BuildShaderDefinitionForMeshData ( InputModelData->Meshes + MeshIndices[Index] );
		ShaderDefs.LightModel = scLightModel_Off;
		crShaderHandle Shader = scAssembleShader ( ShaderDefs );

		crSetRenderCommandShader ( *CommandArray + Index, Shader );
		BuildRenderCommandForMeshData ( *CommandArray + Index, InputModelData->Meshes + MeshIndices[Index], Shader );
		}
	return true;
	}

ResourceHandle LoadModelFileIntoResourceHub ( const char *Filename, const char *ResourceName )
	{
	int FileIndex = ThreadedFileLoader_LoadFile ( Filename );
	if ( FileIndex == -1 )
		return ResourceHandle_Invalid;
	ThreadedFileLoader_WaitUntilAllLoaded();
	ThreadedFileLoader_State State = ThreadedFileLoader_GetFileLoadState ( FileIndex );
	if ( State != HUD_STATE_LOADED_OK )
		return ResourceHandle_Invalid;

	char *OwningPath = alloca ( strlen ( Filename ) + 1 );
	strcpy ( OwningPath, Filename );
	char *LastSlash = strrchr ( OwningPath, '/' );
	if ( LastSlash != NULL )
		*LastSlash = 0;

	DataFileContentDescriptor *ModelFileData = ThreadedFileLoader_GetFileContentDescriptor ( FileIndex );

	char FullTexturePath[1024];
	// Load all external textures that are referenced..
	for ( unsigned MaterialIndex = 0; MaterialIndex < ModelFileData->MaterialCount; ++MaterialIndex )
		{
		for ( unsigned Type = 0; Type < MatTexType_Count; ++Type )
			{
			if ( ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName == NULL ) continue;
			snprintf ( FullTexturePath, sizeof ( FullTexturePath ), "%s/%s", OwningPath, ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName );
			ThreadedFileLoader_LoadFile ( FullTexturePath );
			}
		}
	ThreadedFileLoader_WaitUntilAllLoaded();// TODO Wait only for these textures...

	for ( unsigned MaterialIndex = 0; MaterialIndex < ModelFileData->MaterialCount; ++MaterialIndex )
		{
		for ( unsigned Type = 0; Type < MatTexType_Count; ++Type )
			{
			if ( ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName == NULL ) continue;
			snprintf ( FullTexturePath, sizeof ( FullTexturePath ), "%s/%s", OwningPath, ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName );

			// TODO Wait for texture finished loading here
			int FileIndex = ThreadedFileLoader_GetResourceFileIndex ( FullTexturePath );
			if ( FileIndex == -1 )
				{
				LOG_ERROR ( "Unable to find loaded file '%s'", FullTexturePath );
				continue;
				}
			DataFileContentDescriptor *TextureFileData = ThreadedFileLoader_GetFileContentDescriptor ( FileIndex );
			if ( TextureFileData->TextureCount != 1 )
				{
				LOG_ERROR ( "Unexpected texture count %u for file '%s'", TextureFileData->TextureCount, FullTexturePath );
				continue;
				}
			for ( unsigned TextureIndex = 0; TextureIndex < TextureFileData->TextureCount; ++TextureIndex )
				{
				ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Texture, ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName );
				ResourceHub_SetTextureDescriptor ( NewResource, &TextureFileData->Textures[TextureIndex] );
				}
			}
		}

	// Load all data referenced in the main file
	for ( unsigned Index = 0; Index < ModelFileData->TextureCount; ++Index )
		{
		ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Texture, ModelFileData->Textures[Index].TextureName );
		ResourceHub_SetTextureDescriptor ( NewResource, &ModelFileData->Textures[Index] );
		}

	for ( unsigned Index = 0; Index < ModelFileData->MaterialCount; ++Index )
		{
		ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Material, ModelFileData->Materials[Index].MaterialName );
		ResourceHub_SetMaterialDescriptor ( NewResource, &ModelFileData->Materials[Index] );
		}

	for ( unsigned Index = 0; Index < ModelFileData->MeshCount; ++Index )
		{
		ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Mesh, ModelFileData->Meshes[Index].MeshName );
		ResourceHub_SetMeshDescriptor ( NewResource, &ModelFileData->Meshes[Index] );
		}

	ModelDescriptor NewModelDescriptor = { 0 };
	NewModelDescriptor.MeshCount = ModelFileData->MeshCount;
	NewModelDescriptor.Meshes = ModelFileData->Meshes;
	NewModelDescriptor.Name = ( char * ) ResourceName;
	NewModelDescriptor.NodeCount = ModelFileData->NodeCount;
	NewModelDescriptor.Nodes = ModelFileData->NodeTree;

	ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Model, ResourceName );
	ResourceHub_SetModelDescriptor ( NewResource, &NewModelDescriptor );
	DestroyDataFileContentDescriptor ( ModelFileData );
	free ( ModelFileData );
	return NewResource;
	}