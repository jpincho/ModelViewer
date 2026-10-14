#include "Utils.h"
#include "ResourceLoading/ThreadedFileLoader.h"
#include <C_CrossRenderer/Shader.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <Platform/Logger.h>
#include <Platform/Platform.h>

scShaderDefinition BuildShaderDefinitionForMeshData ( const MeshData *InputMeshData )
	{
	scShaderDefinition ShaderDefinition = { 0 };

	// Where to get diffuse component from
	if ( ( InputMeshData->Material.Textures[MatTexType_Diffuse].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.DiffuseSource = scSource_Texture;
	else if ( InputMeshData->Color != crShaderBufferHandle_Invalid )
		ShaderDefinition.DiffuseSource = scSource_Attribute;
	else
		ShaderDefinition.DiffuseSource = scSource_Uniform;

	// Where to get specular component from
	if ( ( InputMeshData->Material.Textures[MatTexType_Specular].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.SpecularSource = scSource_Texture;
	else if ( InputMeshData->Color != crShaderBufferHandle_Invalid )
		ShaderDefinition.SpecularSource = scSource_Attribute;
	else
		ShaderDefinition.SpecularSource = scSource_Uniform;

	// Where to get shininess component from
	if ( ( InputMeshData->Material.Textures[MatTexType_Shininess].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.ShininessSource = scSource_Texture;
	else if ( InputMeshData->Shininess != crShaderBufferHandle_Invalid )
		ShaderDefinition.ShininessSource = scSource_Attribute;
	else
		ShaderDefinition.ShininessSource = scSource_Uniform;

	// Where to get emissiveness component from
	if ( ( InputMeshData->Material.Textures[MatTexType_Emissive].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		ShaderDefinition.EmissivenessSource = scSource_Texture;
	else if ( InputMeshData->Emissive != crShaderBufferHandle_Invalid )
		ShaderDefinition.EmissivenessSource = scSource_Attribute;
	else
		ShaderDefinition.EmissivenessSource = scSource_Uniform;

	// Where to get normal vector from
	if ( ( InputMeshData->Material.Textures[MatTexType_Normal].Handle != crTextureHandle_Invalid ) && ( InputMeshData->TexCoordStream.BufferHandle != crShaderBufferHandle_Invalid ) ) // && (InputMeshData->BitangentStream.BufferHandle!=crShaderBufferHandle_Invalid) && (InputMeshData->TangentStream.BufferHandle != crShaderBufferHandle_Invalid))
		ShaderDefinition.NormalSource = scSource_Texture;
	else if ( InputMeshData->Normal != crShaderBufferHandle_Invalid )
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
	vec4 Diffuse = { InputMeshData->Material.Diffuse.x, InputMeshData->Material.Diffuse.y, InputMeshData->Material.Diffuse.z, 1.0f };
	vec4 Emissive = { InputMeshData->Material.Emissive.x, InputMeshData->Material.Emissive.y, InputMeshData->Material.Emissive.z, 1.0f };
	vec4 Specular = { InputMeshData->Material.Specular.x, InputMeshData->Material.Specular.y, InputMeshData->Material.Specular.z, 1.0f };
	crSetRenderCommandUniformFloat4Value ( Command, crGetShaderUniformHandle ( Shader, scGetStandardMaterialUniformName ( MatTexType_Diffuse ) ), Diffuse );
	crSetRenderCommandUniformFloat4Value ( Command, crGetShaderUniformHandle ( Shader, scGetStandardMaterialUniformName ( MatTexType_Emissive ) ), Emissive );
	crSetRenderCommandUniformFloatValue ( Command, crGetShaderUniformHandle ( Shader, scGetStandardMaterialUniformName ( MatTexType_Shininess ) ), InputMeshData->Material.Shininess );
	crSetRenderCommandUniformFloat4Value ( Command, crGetShaderUniformHandle ( Shader, scGetStandardMaterialUniformName ( MatTexType_Specular ) ), Specular );

	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Position ) ), InputMeshData->PositionStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_TextureCoordinate ) ), InputMeshData->TexCoordStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Normal ) ), InputMeshData->NormalStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Tangent ) ), InputMeshData->TangentStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Bitangent ) ), InputMeshData->BitangentStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Color ) ), InputMeshData->ColorStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Shininess ) ), InputMeshData->ShininessStream );
	crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_Emissiveness ) ), InputMeshData->EmissiveStream );
	if ( ( InputMeshData->BoneCount != 0 ) && ( InputMeshData->BoneIndexStream.BufferHandle != crShaderBufferHandle_Invalid ) && ( InputMeshData->BoneWeightStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		{
		crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_BoneIndex ) ), InputMeshData->BoneIndexStream );
		crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_BoneWeight ) ), InputMeshData->BoneWeightStream );
		}
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

bool BuildModelRenderData ( ModelRenderData *RenderData, const ModelData *InputModelData )
	{
	memset ( RenderData, 0, sizeof ( ModelRenderData ) );

	unsigned TotalMeshCount = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < InputModelData->NodeCount; ++NodeIndex )
		TotalMeshCount += InputModelData->NodeTree[NodeIndex].MeshIndexCount;

	if ( TotalMeshCount == 0 )
		return false;

	RenderData->Count = TotalMeshCount;
	RenderData->Commands = calloc ( TotalMeshCount, sizeof ( crRenderCommand ) );
	RenderData->NodeIndices = calloc ( TotalMeshCount, sizeof ( unsigned ) );
	if ( ( RenderData->Commands == NULL ) || ( RenderData->NodeIndices == NULL ) )
		{
		DestroyModelRenderData ( RenderData );
		return false;
		}

	unsigned *MeshIndices = alloca ( TotalMeshCount * sizeof ( unsigned ) );
	unsigned CurrentPosition = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < InputModelData->NodeCount; ++NodeIndex )
		{
		for ( unsigned MeshIndex = 0; MeshIndex < InputModelData->NodeTree[NodeIndex].MeshIndexCount; ++MeshIndex )
			{
			const unsigned MeshArrayIndex = InputModelData->NodeTree[NodeIndex].MeshIndices[MeshIndex];
			if ( MeshArrayIndex >= InputModelData->MeshCount )
				{
				LOG_ERROR ( "Node '%s' references invalid mesh index %u", InputModelData->NodeTree[NodeIndex].NodeName, MeshArrayIndex );
				continue;
				}
			MeshIndices[CurrentPosition] = MeshArrayIndex;
			RenderData->NodeIndices[CurrentPosition] = NodeIndex;
			++CurrentPosition;
			}
		}

	RenderData->Count = CurrentPosition;
	if ( RenderData->Count == 0 )
		{
		DestroyModelRenderData ( RenderData );
		return false;
		}

	// If I'm unable to generate the shader for some mesh, keep going with the other meshes
	unsigned Written = 0;
	for ( unsigned Index = 0; Index < RenderData->Count; ++Index )
		{
		scShaderDefinition ShaderDefs = BuildShaderDefinitionForMeshData ( InputModelData->Meshes + MeshIndices[Index] );
		ShaderDefs.LightModel = scLightModel_Off;
		crShaderHandle Shader = scAssembleShader ( ShaderDefs );
		if ( Shader == crShaderHandle_Invalid )
			{
			LOG_ERROR ( "Unable to assemble shader for mesh %u", MeshIndices[Index] );
			continue;
			}

		RenderData->NodeIndices[Written] = RenderData->NodeIndices[Index];
		crSetRenderCommandShader ( RenderData->Commands + Written, Shader );
		BuildRenderCommandForMeshData ( RenderData->Commands + Written, InputModelData->Meshes + MeshIndices[Index], Shader );
		++Written;
		}
	RenderData->Count = Written;
	if ( RenderData->Count == 0 )
		{
		DestroyModelRenderData ( RenderData );
		return false;
		}
	RenderData->OriginalModelData = InputModelData;
	return true;
	}

void UpdateModelRenderData ( const ModelRenderData *RenderData, BaseTransformStruct *Transform, const float Scale, const mat4 ViewMatrix, const mat4 ProjectionMatrix )
	{
	mat4 GlobalMatrix;
	GetTransformationMatrix ( Transform, &GlobalMatrix );

	mat4 ScaleMatrix;
	math_mat4_set_scale_matrix ( &ScaleMatrix, ( vec3 ) { Scale, Scale, Scale } );
	mat4 ScaledGlobalMatrix;
	math_mat4_multiply ( &ScaledGlobalMatrix, ScaleMatrix, GlobalMatrix );

	mat4 *NodeWorldMatrices = alloca ( RenderData->OriginalModelData->NodeCount * sizeof ( mat4 ) );
	ComputeMeshTreeNodeWorldMatrices ( RenderData->OriginalModelData->NodeTree, RenderData->OriginalModelData->NodeCount, NodeWorldMatrices );

	for ( unsigned Index = 0; Index < RenderData->Count; ++Index )
	{
		const unsigned NodeIndex = RenderData->NodeIndices[Index];
		mat4 ModelMatrix;
		math_mat4_multiply ( &ModelMatrix, ScaledGlobalMatrix, NodeWorldMatrices[NodeIndex] );
		
		crRenderCommand *Command = &RenderData->Commands[Index];
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ModelMatrix" ), ModelMatrix );
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ViewMatrix" ), ViewMatrix );
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ProjectionMatrix" ), ProjectionMatrix );
		
		mat3 InvertedModelMatrix;
		math_mat3_from_mat4 ( &InvertedModelMatrix, ModelMatrix );
		math_mat3_invert ( &InvertedModelMatrix, InvertedModelMatrix );
		mat3 NormalMatrix;
		math_mat3_transpose ( &NormalMatrix, InvertedModelMatrix );
		crSetRenderCommandUniformMatrix3Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_NormalMatrix" ), NormalMatrix );
		
		// Set default bone matrices to identity
		mat4 BindBoneMatrix;
		math_mat4_set_identity_matrix ( &BindBoneMatrix );
		for ( unsigned BoneIndex = 0; ; ++BoneIndex )
			{
			char BoneUniformName[64];
			snprintf ( BoneUniformName, sizeof ( BoneUniformName ), "uBoneMatrices[%u]", BoneIndex );
			crShaderUniformHandle BoneHandle = crGetShaderUniformHandle ( Command->Shader, BoneUniformName );
			if ( BoneHandle == crShaderUniformHandle_Invalid )
				break;
			crSetRenderCommandUniformMatrix4Value ( Command, BoneHandle, BindBoneMatrix );
			}
		}
	}

void DestroyModelRenderData ( ModelRenderData *RenderData )
	{
	if ( RenderData->Commands != NULL )
		{
		for ( unsigned Index = 0; Index < RenderData->Count; ++Index )
			SAFE_DELETE_SHADER ( RenderData->Commands[Index].Shader );
		}
	SAFE_DEL_C ( RenderData->Commands );
	SAFE_DEL_C ( RenderData->NodeIndices );
	RenderData->Count = 0;
	RenderData->OriginalModelData = NULL;
	}

ResourceHandle LoadModelFileIntoResourceHub ( const char *Filename, const char *ResourceName )
	{
	int FileIndex = ThreadedFileLoader_LoadFile ( Filename );
	if ( FileIndex == -1 )
		{
		LOG_ERROR ( "ThreadedFileLoader_LoadFile failed" );
		return ResourceHandle_Invalid;
		}
	ThreadedFileLoader_WaitUntilAllLoaded();
	ThreadedFileLoader_State State = ThreadedFileLoader_GetFileLoadState ( FileIndex );
	if ( State != HUD_STATE_LOADED_OK )
		{
		LOG_ERROR ( "ThreadedFileLoader_GetFileLoadState returned %u", State );
		return ResourceHandle_Invalid;
		}

	char *OwningPath = alloca ( strlen ( Filename ) + 1 );
	strcpy ( OwningPath, Filename );
	char *LastSlash = strrchr ( OwningPath, PLATFORM_PATH_SEPARATOR );
	if ( LastSlash != NULL )
		*LastSlash = 0;

	DataFileContentDescriptor *ModelFileData = ThreadedFileLoader_GetFileContentDescriptor ( FileIndex );
	if ( ModelFileData == NULL )
		return ResourceHandle_Invalid;

	char FullTexturePath[1024];
	// Load all external textures that are referenced..
	for ( unsigned MaterialIndex = 0; MaterialIndex < ModelFileData->MaterialCount; ++MaterialIndex )
		{
		for ( unsigned Type = 0; Type < MatTexType_Count; ++Type )
			{
			if ( ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName == NULL ) continue;
			snprintf ( FullTexturePath, sizeof ( FullTexturePath ), "%s%c%s", OwningPath, PLATFORM_PATH_SEPARATOR, ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName );
			ThreadedFileLoader_LoadFile ( FullTexturePath );
			}
		}
	ThreadedFileLoader_WaitUntilAllLoaded();// TODO Wait only for these textures...

	for ( unsigned MaterialIndex = 0; MaterialIndex < ModelFileData->MaterialCount; ++MaterialIndex )
		{
		for ( unsigned Type = 0; Type < MatTexType_Count; ++Type )
			{
			if ( ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName == NULL ) continue;
			snprintf ( FullTexturePath, sizeof ( FullTexturePath ), "%s%c%s", OwningPath, PLATFORM_PATH_SEPARATOR, ModelFileData->Materials[MaterialIndex].Textures[Type].TextureName );

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
	ThreadedFileLoader_UnloadResourceFile ( FileIndex );
	return NewResource;
	}

void ComputeMeshTreeNodeWorldMatrices ( const MeshTreeNode *NodeTree, const unsigned NodeCount, mat4 *WorldMatrices )
	{
	for ( unsigned Index = 0; Index < NodeCount; ++Index )
		{
		const int Parent = NodeTree[Index].ParentNode;
		if ( Parent < 0 )
			math_mat4_copy ( &WorldMatrices[Index], NodeTree[Index].DefaultTransformationMatrix );
		else if ( ( unsigned ) Parent < Index )
			math_mat4_multiply ( &WorldMatrices[Index], WorldMatrices[Parent], NodeTree[Index].DefaultTransformationMatrix );
		else
			{
			mat4 Accumulated;
			math_mat4_copy ( &Accumulated, NodeTree[Index].DefaultTransformationMatrix );
			int Walk = Parent;
			while ( Walk >= 0 )
				{
				mat4 Combined;
				math_mat4_multiply ( &Combined, NodeTree[Walk].DefaultTransformationMatrix, Accumulated );
				math_mat4_copy ( &Accumulated, Combined );
				Walk = NodeTree[Walk].ParentNode;
				}
			math_mat4_copy ( &WorldMatrices[Index], Accumulated );
			}
		}
	}
