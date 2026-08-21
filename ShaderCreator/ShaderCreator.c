#include "ShaderCreator.h"
#include <Platform/Logger.h>
#include <stdio.h>
#include <string.h>

static bool ShaderCreatorInitialized = false;

static bool SourceUsesTexture ( const scMaterialSource Source )
	{
	return Source == scSource_Texture;
	}

static bool SourceUsesColorAttribute ( const scMaterialSource Source )
	{
	return Source == scSource_Attribute;
	}

static scPin AddVec4Source ( scShaderGraph *Graph, const scMaterialSource Source, const MatTexType Type, const scNodeId UV, const scNodeId ColorAttr, const scPin OffValue )
	{
	switch ( Source )
		{
		case scSource_Uniform:
			{
			const scNodeId Uniform = scGraph_AddUniform ( Graph, scGetStandardMaterialUniformName ( Type ), scValue_Vec4 );
			return scNodePin ( Uniform );
			}
		case scSource_Attribute:
			return scNodePin ( ColorAttr );
		case scSource_Texture:
			{
			const scNodeId Sampler = scGraph_AddUniform ( Graph, scGetStandardTextureUniformName ( Type ), scValue_Sampler2D );
			const scNodeId Sample = scGraph_AddTextureSample ( Graph, scNodePin ( Sampler ), scNodePin ( UV ) );
			return scNodePin ( Sample );
			}
		case scSource_Off:
		default:
			return OffValue;
		}
	}

static scPin AddFloatSource ( scShaderGraph *Graph, const scMaterialSource Source, const MatTexType Type, const scNodeId UV, const scNodeId ShininessAttr, const scPin OffValue )
	{
	switch ( Source )
		{
		case scSource_Uniform:
			{
			const scNodeId Uniform = scGraph_AddUniform ( Graph, scGetStandardMaterialUniformName ( Type ), scValue_Float );
			return scNodePin ( Uniform );
			}
		case scSource_Attribute:
			return scNodePin ( ShininessAttr );
		case scSource_Texture:
			{
			const scNodeId Sampler = scGraph_AddUniform ( Graph, scGetStandardTextureUniformName ( Type ), scValue_Sampler2D );
			const scNodeId Sample = scGraph_AddTextureSample ( Graph, scNodePin ( Sampler ), scNodePin ( UV ) );
			const scNodeId Channel = scGraph_AddSwizzle ( Graph, scNodePin ( Sample ), "a" );
			return scNodePin ( Channel );
			}
		case scSource_Off:
		default:
			return OffValue;
		}
	}

static scPin AddWorldNormal ( scShaderGraph *Graph, const scShaderDefinition Definition, const scPin Model, const scPin BoneMatrix, const scNodeId UV )
	{
	const scNodeId NormalAttr = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Normal ), scValue_Vec3 );
	if ( Definition.NormalSource == scSource_Texture )
		{
		const scNodeId Tangent = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Tangent ), scValue_Vec3 );
		const scNodeId Bitangent = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Bitangent ), scValue_Vec3 );
		const scNodeId TBN = scGraph_AddTBN ( Graph, scNodePin ( Tangent ), scNodePin ( Bitangent ), scNodePin ( NormalAttr ), Model, BoneMatrix );
		scGraph_SetVaryingName ( Graph, TBN, "v_TBN" );
		const scNodeId Sampler = scGraph_AddUniform ( Graph, scGetStandardTextureUniformName ( MatTexType_Normal ), scValue_Sampler2D );
		const scNodeId Mapped = scGraph_AddNormalMap ( Graph, scNodePin ( Sampler ), scNodePin ( UV ), scNodePin ( TBN ) );
		return scNodePin ( Mapped );
		}

	const scNodeId ModelBone = scGraph_AddMultiply ( Graph, Model, BoneMatrix );
	const scNodeId ModelBone3 = scGraph_AddMat3Cast ( Graph, scNodePin ( ModelBone ) );
	const scNodeId Transformed = scGraph_AddMultiply ( Graph, scNodePin ( ModelBone3 ), scNodePin ( NormalAttr ) );
	const scNodeId Normalized = scGraph_AddNormalize ( Graph, scNodePin ( Transformed ) );
	scGraph_SetVaryingName ( Graph, Normalized, "v_Normal" );
	return scNodePin ( Normalized );
	}

scShaderGraph *scGraph_BuildFromDefinition ( const scShaderDefinition Definition )
	{
	scShaderGraph *Graph = scGraph_Create ();
	if ( Graph == NULL )
		return NULL;

	const bool UsesTextures = SourceUsesTexture ( Definition.DiffuseSource ) ||
	                          SourceUsesTexture ( Definition.SpecularSource ) ||
	                          SourceUsesTexture ( Definition.ShininessSource ) ||
	                          SourceUsesTexture ( Definition.EmissivenessSource ) ||
	                          SourceUsesTexture ( Definition.NormalSource );
	const bool UsesColorAttribute = SourceUsesColorAttribute ( Definition.DiffuseSource ) ||
	                                SourceUsesColorAttribute ( Definition.SpecularSource );
	const bool UseLighting = ( Definition.LightModel != scLightModel_Off ) && ( Definition.NormalSource != scSource_Off );

	const scNodeId Position = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Position ), scValue_Vec3 );
	const scNodeId Model = scGraph_AddUniform ( Graph, "u_ModelMatrix", scValue_Mat4 );
	const scNodeId View = scGraph_AddUniform ( Graph, "u_ViewMatrix", scValue_Mat4 );
	const scNodeId Projection = scGraph_AddUniform ( Graph, "u_ProjectionMatrix", scValue_Mat4 );

	scPin LocalPosition;
	scPin BoneMatrix;
	if ( Definition.Skinning && ( Definition.BoneCount > 0 ) )
		{
		const scNodeId BoneIndex = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_BoneIndex ), scValue_UVec4 );
		const scNodeId BoneWeight = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_BoneWeight ), scValue_Vec4 );
		const scNodeId Bones = scGraph_AddUniformArray ( Graph, "u_Bones", scValue_Mat4, Definition.BoneCount );
		const scNodeId Skin = scGraph_AddSkinning ( Graph, scNodePin ( Position ), scNodePin ( BoneIndex ), scNodePin ( BoneWeight ), scNodePin ( Bones ) );
		BoneMatrix = scMakePin ( Skin, scSkinningOut_BoneMatrix );
		LocalPosition = scMakePin ( Skin, scSkinningOut_Position );
		}
	else
		{
		const scNodeId One = scGraph_AddConstantFloat ( Graph, 1.0f );
		LocalPosition = scNodePin ( scGraph_AddComposeVec4 ( Graph, scNodePin ( Position ), scNodePin ( One ) ) );
		BoneMatrix = scNodePin ( UseLighting ? scGraph_AddIdentityMat4 ( Graph ) : scNodeId_Invalid );
		}

	const scNodeId WorldPosition4 = scGraph_AddMultiply ( Graph, scNodePin ( Model ), LocalPosition );
	const scNodeId ViewPosition4 = scGraph_AddMultiply ( Graph, scNodePin ( View ), scNodePin ( WorldPosition4 ) );
	const scNodeId ClipPosition = scGraph_AddMultiply ( Graph, scNodePin ( Projection ), scNodePin ( ViewPosition4 ) );
	scGraph_AddVertexPosition ( Graph, scNodePin ( ClipPosition ) );

	scNodeId UV = scNodeId_Invalid;
	if ( UsesTextures )
		{
		UV = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_TextureCoordinate ), scValue_Vec2 );
		scGraph_SetVaryingName ( Graph, UV, "v_TexCoord" );
		}

	scNodeId ColorAttr = scNodeId_Invalid;
	if ( UsesColorAttribute )
		{
		ColorAttr = scGraph_AddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Color ), scValue_Vec4 );
		scGraph_SetVaryingName ( Graph, ColorAttr, "v_Color" );
		}

	const scPin White = scNodePin ( scGraph_AddConstantVec4 ( Graph, 1.0f, 1.0f, 1.0f, 1.0f ) );
	const scPin Diffuse = AddVec4Source ( Graph, Definition.DiffuseSource, MatTexType_Diffuse, UV, ColorAttr, White );

	scPin FinalColor = Diffuse;
	if ( UseLighting )
		{
		const scNodeId WorldPosition3 = scGraph_AddSwizzle ( Graph, scNodePin ( WorldPosition4 ), "xyz" );
		scGraph_SetVaryingName ( Graph, WorldPosition3, "v_FragPosition" );
		const scPin SpecularOff = Diffuse;
		const scPin Specular = AddVec4Source ( Graph, Definition.SpecularSource, MatTexType_Specular, UV, ColorAttr, SpecularOff );
		scNodeId ShininessAttr = scNodeId_Invalid;
		if ( Definition.ShininessSource == scSource_Attribute )
			{
			ShininessAttr = scGraph_AddAttribute ( Graph, "a_Shininess", scValue_Float );
			scGraph_SetVaryingName ( Graph, ShininessAttr, "v_Shininess" );
			}
		const scPin DefaultShininess = scNodePin ( scGraph_AddConstantFloat ( Graph, 0.6f ) );
		const scPin Shininess = AddFloatSource ( Graph, Definition.ShininessSource, MatTexType_Shininess, UV, ShininessAttr, DefaultShininess );
		const scPin Normal = AddWorldNormal ( Graph, Definition, scNodePin ( Model ), BoneMatrix, UV );
		const scNodeId ViewPosition = scGraph_AddUniform ( Graph, "u_ViewPosition", scValue_Vec3 );
		const scNodeId Lit = scGraph_AddBlinnPhong ( Graph, Diffuse, Specular, Shininess, Normal, scNodePin ( WorldPosition3 ), scNodePin ( ViewPosition ) );
		FinalColor = scNodePin ( Lit );

		if ( Definition.EmissivenessSource != scSource_Off )
			{
			scNodeId EmissiveAttr = scNodeId_Invalid;
			if ( Definition.EmissivenessSource == scSource_Attribute )
				{
				EmissiveAttr = scGraph_AddAttribute ( Graph, "a_Emissiveness", scValue_Vec4 );
				scGraph_SetVaryingName ( Graph, EmissiveAttr, "v_Emissiveness" );
				}
			const scPin Black = scNodePin ( scGraph_AddConstantVec4 ( Graph, 0.0f, 0.0f, 0.0f, 0.0f ) );
			const scPin Emissive = AddVec4Source ( Graph, Definition.EmissivenessSource, MatTexType_Emissive, UV, EmissiveAttr, Black );
			FinalColor = scNodePin ( scGraph_AddAdd ( Graph, FinalColor, Emissive ) );
			}
		}

	scGraph_AddFragmentColor ( Graph, FinalColor );
	return Graph;
	}

bool scInitializeShaders ( void )
	{
	ShaderCreatorInitialized = true;
	return true;
	}

bool scShutdownShaders ( void )
	{
	ShaderCreatorInitialized = false;
	return true;
	}

crShaderHandle scAssembleShader ( const scShaderDefinition NewDefinition )
	{
	if ( ShaderCreatorInitialized == false )
		scInitializeShaders ();

	if ( ( NewDefinition.Skinning ) && ( NewDefinition.BoneCount == 0 ) )
		{
		LOG_ERROR ( "Invalid bone count for shader creator" );
		return crShaderHandle_Invalid;
		}

	crShaderCode Code;
	memset ( &Code, 0, sizeof ( Code ) );
	scCreateShaderCode ( NewDefinition, &Code );
	if ( ( Code.VertexShader == NULL ) || ( Code.FragmentShader == NULL ) )
		{
		SAFE_DEL_C ( Code.VertexShader );
		SAFE_DEL_C ( Code.FragmentShader );
		SAFE_DEL_C ( Code.GeometryShader );
		return crShaderHandle_Invalid;
		}

	crShaderHandle Handle = crCreateShader ( Code );
	SAFE_DEL_C ( Code.VertexShader );
	SAFE_DEL_C ( Code.FragmentShader );
	SAFE_DEL_C ( Code.GeometryShader );
	return Handle;
	}

void scCreateShaderCode ( const scShaderDefinition NewRequirements, crShaderCode *OutputCode )
	{
	OutputCode->VertexShader = NULL;
	OutputCode->FragmentShader = NULL;
	OutputCode->GeometryShader = NULL;

	scShaderGraph *Graph = scGraph_BuildFromDefinition ( NewRequirements );
	if ( Graph == NULL )
		{
		LOG_ERROR ( "Unable to build shader graph from definition" );
		return;
		}
	if ( scGraph_GenerateCode ( Graph, OutputCode ) == false )
		{
		LOG_ERROR ( "Unable to generate GLSL from shader graph" );
		SAFE_DEL_C ( OutputCode->VertexShader );
		SAFE_DEL_C ( OutputCode->FragmentShader );
		SAFE_DEL_C ( OutputCode->GeometryShader );
		}
	scGraph_Destroy ( Graph );
	}

const char *scGetStandardTextureUniformName ( const MatTexType Type )
	{
	static char Names[MatTexType_Count][50] = { 0 };
	if ( Names[Type][0] == 0 )
		{
		snprintf ( Names[Type], 50, "u_%sTexture", Stringify_MatTexType ( Type ) );
		}
	return Names[Type];
	}

const char *scGetStandardMaterialUniformName ( const MatTexType Type )
	{
	static char Names[MatTexType_Count][50] = { 0 };
	if ( Names[Type][0] == 0 )
		{
		snprintf ( Names[Type], 50, "u_%s", Stringify_MatTexType ( Type ) );
		}
	return Names[Type];
	}

const char *scGetStandardAttributeName ( const scShaderAttributeType Type )
	{
	static char Names[scAttribute_Count][50] = { 0 };
	if ( Names[Type][0] == 0 )
		{
		snprintf ( Names[Type], 50, "a_%s", Stringify_ShaderAttributeType ( Type ) );
		}
	return Names[Type];
	}

const char *Stringify_ShaderAttributeType ( const scShaderAttributeType Value )
	{
#define STRINGIFY(X) case scAttribute_##X: return #X;
	switch ( Value )
		{
			STRINGIFY ( Position );
			STRINGIFY ( TextureCoordinate );
			STRINGIFY ( Normal );
			STRINGIFY ( Color );
			STRINGIFY ( Tangent );
			STRINGIFY ( Bitangent );
			STRINGIFY ( BoneIndex );
			STRINGIFY ( BoneWeight );
		}
#undef STRINGIFY
	return "Invalid scShaderAttributeType";
	}
