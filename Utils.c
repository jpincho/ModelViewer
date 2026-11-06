#include "Utils.h"
#include "ResourceLoading/ThreadedFileLoader.h"
#include <Platform/Logger.h>
#include <float.h>

scShaderDefinition BuildShaderDefinitionForMeshData ( const struct MeshData *InputMeshData )
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
		ShaderDefinition.BoneCount = RoundUpToNearestMultiple ( InputMeshData->BoneCount, 64 );
		}
	return ShaderDefinition;
	}

bool BuildRenderCommandForMeshData ( crRenderCommand *Command, const struct MeshData *InputMeshData, const crShaderHandle Shader )
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
	crSetRenderCommandIndexShaderBufferBinding ( Command, InputMeshData->IndexStream );

	for ( unsigned TypeIterator = 0; TypeIterator < MatTexType_Count; ++TypeIterator )
		{
		if ( InputMeshData->Material.Textures[TypeIterator].Handle )
			{
			crSetRenderCommandTextureBinding ( Command, crGetShaderUniformHandle ( Shader, scGetStandardTextureUniformName ( TypeIterator ) ), InputMeshData->Material.Textures[TypeIterator] );
			}
		}

	// Skinning
	if ( ( InputMeshData->BoneCount != 0 ) && ( InputMeshData->BoneIndexStream.BufferHandle != crShaderBufferHandle_Invalid ) && ( InputMeshData->BoneWeightStream.BufferHandle != crShaderBufferHandle_Invalid ) )
		{
		crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_BoneIndex ) ), InputMeshData->BoneIndexStream );
		crSetRenderCommandShaderBufferBinding ( Command, crGetShaderAttributeHandle ( Shader, scGetStandardAttributeName ( scAttribute_BoneWeight ) ), InputMeshData->BoneWeightStream );
		}

	Command->Primitive = crTriangleList;
	Command->StartVertex = 0;
	Command->VertexCount = InputMeshData->VertexCount;
	Command->InstanceCount = 1;
	crSetRenderStateToDefault ( &Command->State );
	Command->State.Culling.Enabled = true;
	Command->State.DepthTest.Enabled = true;
	return crSanityCheckRenderCommand ( Command ) == 0;
	}

/*
float CalculateScaleToFit ( const struct AABB BBox, const float CameraDistance, const float VerticalFOV, const float AspectRatio )
{
vec3 Delta = math_subtract_vec3_from_vec3 ( BBox.Max, BBox.Min );
float Radius = 0.5f * math_vec3_length ( Delta );
float HalfVerticalFOV = VerticalFOV * 0.5f;
float HalfHorizontalFOV = atanf ( tanf ( HalfVerticalFOV ) * AspectRatio );

float SmallerFOV = fminf ( HalfVerticalFOV, HalfHorizontalFOV );

return CameraDistance * sinf ( SmallerFOV ) / Radius;
}
*/
float CalculateScaleToFit2 ( const struct AABB BBox, const float CameraDistance, const float VerticalFOV, const float AspectRatio )
	{
	float tan_y = tanf ( VerticalFOV * 0.5f );
	float tan_x = tan_y * AspectRatio;

	float max_scale = FLT_MAX;

	vec3 Corners[8];
	AABB_CalculateCorners ( BBox, Corners );
	for ( int i = 0; i < 8; ++i )
		{
		float x = fabsf ( Corners[i].x );
		float y = fabsf ( Corners[i].y );
		float z = Corners[i].z;

		float dx = x - tan_x * z;
		float dy = y - tan_y * z;

		if ( dx > 0.0f )
			{
			float sx = CameraDistance * tan_x / dx;
			if ( sx < max_scale )
				max_scale = sx;
			}

		if ( dy > 0.0f )
			{
			float sy = CameraDistance * tan_y / dy;
			if ( sy < max_scale )
				max_scale = sy;
			}
		}

	return max_scale == FLT_MAX ? 1.0f : max_scale;
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

	struct DataFileContentDescriptor *ModelFileData = ThreadedFileLoader_GetFileContentDescriptor ( FileIndex );
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
			struct DataFileContentDescriptor *TextureFileData = ThreadedFileLoader_GetFileContentDescriptor ( FileIndex );
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
		char UniqueName[256];
		const char *OriginalName = ModelFileData->Meshes[Index].MeshName != NULL ? ModelFileData->Meshes[Index].MeshName : "";
		snprintf ( UniqueName, sizeof ( UniqueName ), "%u:%s", Index, OriginalName );
		char *UniqueCopy = strdup ( UniqueName );
		if ( UniqueCopy == NULL )
			{
			LOG_ERROR ( "Unable to allocate a unique mesh name" );
			return ResourceHandle_Invalid;
			}
		free ( ModelFileData->Meshes[Index].MeshName );
		ModelFileData->Meshes[Index].MeshName = UniqueCopy;
		}

	for ( unsigned Index = 0; Index < ModelFileData->MeshCount; ++Index )
		{
		ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Mesh, ModelFileData->Meshes[Index].MeshName );
		ResourceHub_SetMeshDescriptor ( NewResource, &ModelFileData->Meshes[Index] );
		}

	for ( unsigned Index = 0; Index < ModelFileData->AnimationCount; ++Index )
		{
		ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Animation, ModelFileData->Animations[Index].AnimationName );
		ResourceHub_SetAnimationDescriptor ( NewResource, &ModelFileData->Animations[Index] );
		}

	struct ModelDescriptor NewModelDescriptor = { 0 };
	NewModelDescriptor.MeshCount = ModelFileData->MeshCount;
	NewModelDescriptor.Meshes = ModelFileData->Meshes;
	NewModelDescriptor.Name = ( char * ) ResourceName;
	NewModelDescriptor.NodeCount = ModelFileData->NodeCount;
	NewModelDescriptor.Nodes = ModelFileData->NodeTree;
	CalculateModelDescriptorAABB ( &NewModelDescriptor );
	ResourceHandle NewResource = ResourceHub_CreateEntry ( ResourceType_Model, ResourceName );
	ResourceHub_SetModelDescriptor ( NewResource, &NewModelDescriptor );
	ThreadedFileLoader_UnloadResourceFile ( FileIndex );
	return NewResource;
	}

void AdjustToFit ( const struct AABB BBox, struct CameraData *Camera, float *Scale )
	{
	// Center BBox around (0,0,0)
	struct AABB CenteredBBox;
	CenteredBBox.Min = math_subtract_vec3_from_vec3 ( BBox.Max, BBox.Min );
	CenteredBBox.Min = math_multiply_vec3_by_scalar ( CenteredBBox.Min, -0.5f );

	CenteredBBox.Max = math_subtract_vec3_from_vec3 ( BBox.Max, BBox.Min );
	CenteredBBox.Max = math_multiply_vec3_by_scalar ( CenteredBBox.Max, 0.5f );

	float Distance = fabsf ( Camera->Transform.Position.z );
	if ( Distance < 0.01f )
		Distance = 4.0f;
	*Scale = CalculateScaleToFit2 ( CenteredBBox, Distance, Camera->PerspectiveParameters.FOV, Camera->PerspectiveParameters.AspectRatio );
	SetCameraPosition ( Camera, ( vec3 ) { 0.0f, CenteredBBox.Max.y * *Scale, Distance } );
	}

unsigned RoundUpToNearestMultiple ( const unsigned Number, const unsigned Multiple )
	{
	assert ( Multiple );
	return ( ( Number + Multiple - 1 ) / Multiple ) * Multiple;
	}