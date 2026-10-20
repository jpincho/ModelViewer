#include "ShaderCreator.h"
#include <ShaderGraph/ShaderGraph.h>
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

static sgNodeHandle Vec3ToVec4 ( const sgGraphHandle Graph, const sgNodeHandle Value, const unsigned ValuePin, const float W )
	{
	sgNodeHandle X = sgAddSwizzle ( Graph, "x", sgValueType_Vec3, sgValueType_Float );
	sgNodeHandle Y = sgAddSwizzle ( Graph, "y", sgValueType_Vec3, sgValueType_Float );
	sgNodeHandle Z = sgAddSwizzle ( Graph, "z", sgValueType_Vec3, sgValueType_Float );
	sgNodeHandle WNode = sgAddConstantFloat ( Graph, W );
	sgNodeHandle Compose = sgAddComposeVector ( Graph, sgValueType_Float, 4 );
	if ( ( sgAddConnection ( Graph, Value, ValuePin, X, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, Value, ValuePin, Y, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, Value, ValuePin, Z, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, X, 0, Compose, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, Y, 0, Compose, 1 ) == false ) ||
	        ( sgAddConnection ( Graph, Z, 0, Compose, 2 ) == false ) ||
	        ( sgAddConnection ( Graph, WNode, 0, Compose, 3 ) == false ) )
		return sgNodeHandle_Invalid;
	return Compose;
	}

static sgNodeHandle MultiplyMat4ByVec4 ( const sgGraphHandle Graph, const sgNodeHandle Matrix, const sgNodeHandle Vector )
	{
	sgNodeHandle Product = sgAddMultiplicationTyped ( Graph, sgValueType_Mat4, sgValueType_Vec4 );
	if ( ( sgAddConnection ( Graph, Matrix, 0, Product, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, Vector, 0, Product, 1 ) == false ) )
		return sgNodeHandle_Invalid;
	return Product;
	}

static sgNodeHandle MultiplyMat3ByVec3 ( const sgGraphHandle Graph, const sgNodeHandle Matrix, const sgNodeHandle Vector, const unsigned VectorPin )
	{
	sgNodeHandle Product = sgAddMultiplicationTyped ( Graph, sgValueType_Mat3, sgValueType_Vec3 );
	if ( ( sgAddConnection ( Graph, Matrix, 0, Product, 0 ) == false ) ||
	        ( sgAddConnection ( Graph, Vector, VectorPin, Product, 1 ) == false ) )
		return sgNodeHandle_Invalid;
	return Product;
	}

static sgNodeHandle AddVec4Source ( const sgGraphHandle Graph, const scMaterialSource Source, const MatTexType Type, const sgNodeHandle UV, const sgNodeHandle Attribute, const sgNodeHandle OffValue )
	{
	switch ( Source )
		{
		case scSource_Uniform:
			return sgAddUniform ( Graph, scGetStandardMaterialUniformName ( Type ), sgValueType_Vec4 );
		case scSource_Attribute:
			return Attribute;
		case scSource_Texture:
			{
			sgNodeHandle Sampler = sgAddUniform ( Graph, scGetStandardTextureUniformName ( Type ), sgValueType_Sampler2D );
			sgNodeHandle Sample = sgAddTextureSample ( Graph );
			if ( ( sgAddConnection ( Graph, Sampler, 0, Sample, 0 ) == false ) ||
			        ( sgAddConnection ( Graph, UV, 0, Sample, 1 ) == false ) )
				return sgNodeHandle_Invalid;
			return Sample;
			}
		case scSource_Off:
		default:
			return OffValue;
		}
	}

static sgNodeHandle AddFloatSource ( const sgGraphHandle Graph, const scMaterialSource Source, const MatTexType Type, const sgNodeHandle UV, const sgNodeHandle Attribute, const sgNodeHandle OffValue )
	{
	switch ( Source )
		{
		case scSource_Uniform:
			return sgAddUniform ( Graph, scGetStandardMaterialUniformName ( Type ), sgValueType_Float );
		case scSource_Attribute:
			return Attribute;
		case scSource_Texture:
			{
			sgNodeHandle Sampler = sgAddUniform ( Graph, scGetStandardTextureUniformName ( Type ), sgValueType_Sampler2D );
			sgNodeHandle Sample = sgAddTextureSample ( Graph );
			sgNodeHandle Channel = sgAddSwizzle ( Graph, "a", sgValueType_Vec4, sgValueType_Float );
			if ( ( sgAddConnection ( Graph, Sampler, 0, Sample, 0 ) == false ) ||
			        ( sgAddConnection ( Graph, UV, 0, Sample, 1 ) == false ) ||
			        ( sgAddConnection ( Graph, Sample, 0, Channel, 0 ) == false ) )
				return sgNodeHandle_Invalid;
			return Channel;
			}
		case scSource_Off:
		default:
			return OffValue;
		}
	}

static sgNodeHandle AddWorldNormal ( const sgGraphHandle Graph, const scShaderDefinition Definition, const sgNodeHandle NormalMatrix, const sgNodeHandle Normal, const unsigned NormalPin, const sgNodeHandle UV )
	{
	if ( Definition.NormalSource == scSource_Texture )
		{
		/* Tangent and bitangent are rotated by the normal matrix. Skinning does not expose the blended bone matrix. */
		sgNodeHandle Tangent = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Tangent ), sgValueType_Vec3 );
		sgNodeHandle Bitangent = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Bitangent ), sgValueType_Vec3 );
		sgNodeHandle WorldTangent = MultiplyMat3ByVec3 ( Graph, NormalMatrix, Tangent, 0 );
		sgNodeHandle WorldBitangent = MultiplyMat3ByVec3 ( Graph, NormalMatrix, Bitangent, 0 );
		sgNodeHandle WorldNormal = MultiplyMat3ByVec3 ( Graph, NormalMatrix, Normal, NormalPin );
		sgNodeHandle TBN = sgAddTBN ( Graph );
		if ( ( sgAddConnection ( Graph, WorldNormal, 0, TBN, 0 ) == false ) ||
		        ( sgAddConnection ( Graph, WorldTangent, 0, TBN, 1 ) == false ) ||
		        ( sgAddConnection ( Graph, WorldBitangent, 0, TBN, 2 ) == false ) )
			return sgNodeHandle_Invalid;

		sgNodeHandle Sampler = sgAddUniform ( Graph, scGetStandardTextureUniformName ( MatTexType_Normal ), sgValueType_Sampler2D );
		sgNodeHandle Sample = sgAddTextureSample ( Graph );
		sgNodeHandle SampleXYZ = sgAddSwizzle ( Graph, "xyz", sgValueType_Vec4, sgValueType_Vec3 );
		sgNodeHandle Mapped = sgAddNormalMap ( Graph );
		if ( ( sgAddConnection ( Graph, Sampler, 0, Sample, 0 ) == false ) ||
		        ( sgAddConnection ( Graph, UV, 0, Sample, 1 ) == false ) ||
		        ( sgAddConnection ( Graph, Sample, 0, SampleXYZ, 0 ) == false ) ||
		        ( sgAddConnection ( Graph, TBN, 0, Mapped, 0 ) == false ) ||
		        ( sgAddConnection ( Graph, SampleXYZ, 0, Mapped, 1 ) == false ) )
			return sgNodeHandle_Invalid;
		return Mapped;
		}

	sgNodeHandle Transformed = MultiplyMat3ByVec3 ( Graph, NormalMatrix, Normal, NormalPin );
	sgNodeHandle Normalized = sgAddNormalize ( Graph );
	if ( sgAddConnection ( Graph, Transformed, 0, Normalized, 0 ) == false )
		return sgNodeHandle_Invalid;
	return Normalized;
	}

static bool FillShaderGraph ( const sgGraphHandle Graph, const scShaderDefinition Definition )
	{
	const bool UsesTextures = SourceUsesTexture ( Definition.DiffuseSource ) ||
	                          SourceUsesTexture ( Definition.SpecularSource ) ||
	                          SourceUsesTexture ( Definition.ShininessSource ) ||
	                          SourceUsesTexture ( Definition.EmissivenessSource ) ||
	                          SourceUsesTexture ( Definition.NormalSource );
	const bool UsesColorAttribute = SourceUsesColorAttribute ( Definition.DiffuseSource ) ||
	                                SourceUsesColorAttribute ( Definition.SpecularSource );
	const bool UseLighting = ( Definition.LightModel != scLightModel_Off ) && ( Definition.NormalSource != scSource_Off );

	sgNodeHandle Position = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Position ), sgValueType_Vec3 );
	sgNodeHandle Model = sgAddUniform ( Graph, "u_ModelMatrix", sgValueType_Mat4 );
	sgNodeHandle View = sgAddUniform ( Graph, "u_ViewMatrix", sgValueType_Mat4 );
	sgNodeHandle Projection = sgAddUniform ( Graph, "u_ProjectionMatrix", sgValueType_Mat4 );
	if ( ( Position == sgNodeHandle_Invalid ) || ( Model == sgNodeHandle_Invalid ) ||
	        ( View == sgNodeHandle_Invalid ) || ( Projection == sgNodeHandle_Invalid ) )
		return false;

	sgNodeHandle LocalPosition = Position;
	unsigned LocalPositionPin = 0;
	sgNodeHandle ShadingNormal = sgNodeHandle_Invalid;
	unsigned ShadingNormalPin = 0;
	if ( UseLighting )
		{
		ShadingNormal = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Normal ), sgValueType_Vec3 );
		if ( ShadingNormal == sgNodeHandle_Invalid )
			return false;
		}

	if ( Definition.Skinning )
		{
		if ( ShadingNormal == sgNodeHandle_Invalid )
			ShadingNormal = sgAddConstantVec3 ( Graph, 0.0f, 0.0f, 1.0f );

		sgNodeHandle BoneIndex = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_BoneIndex ), sgValueType_UVec4 );
		sgNodeHandle BoneWeight = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_BoneWeight ), sgValueType_Vec4 );
		sgNodeHandle Skin = sgAddSkinning ( Graph );
		if ( ( ShadingNormal == sgNodeHandle_Invalid ) || ( BoneIndex == sgNodeHandle_Invalid ) ||
		        ( BoneWeight == sgNodeHandle_Invalid ) || ( Skin == sgNodeHandle_Invalid ) ||
		        ( sgSetSkinningBoneCount ( Graph, Skin, Definition.BoneCount ) == false ) ||
		        ( sgAddConnection ( Graph, Position, 0, Skin, 0 ) == false ) ||
		        ( sgAddConnection ( Graph, ShadingNormal, ShadingNormalPin, Skin, 1 ) == false ) ||
		        ( sgAddConnection ( Graph, BoneIndex, 0, Skin, 2 ) == false ) ||
		        ( sgAddConnection ( Graph, BoneWeight, 0, Skin, 3 ) == false ) )
			return false;
		LocalPosition = Skin;
		LocalPositionPin = 0;
		ShadingNormal = Skin;
		ShadingNormalPin = 1;
		}

	sgNodeHandle LocalPosition4 = Vec3ToVec4 ( Graph, LocalPosition, LocalPositionPin, 1.0f );
	sgNodeHandle WorldPosition4 = MultiplyMat4ByVec4 ( Graph, Model, LocalPosition4 );
	sgNodeHandle ViewPosition4 = MultiplyMat4ByVec4 ( Graph, View, WorldPosition4 );
	sgNodeHandle ClipPosition = MultiplyMat4ByVec4 ( Graph, Projection, ViewPosition4 );
	sgNodeHandle VertexPosition = sgAddVertexPosition ( Graph );
	if ( ( LocalPosition4 == sgNodeHandle_Invalid ) || ( WorldPosition4 == sgNodeHandle_Invalid ) ||
	        ( ViewPosition4 == sgNodeHandle_Invalid ) || ( ClipPosition == sgNodeHandle_Invalid ) ||
	        ( sgAddConnection ( Graph, ClipPosition, 0, VertexPosition, 0 ) == false ) )
		return false;

	sgNodeHandle UV = sgNodeHandle_Invalid;
	if ( UsesTextures )
		{
		UV = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_TextureCoordinate ), sgValueType_Vec2 );
		if ( UV == sgNodeHandle_Invalid )
			return false;
		}

	sgNodeHandle ColorAttr = sgNodeHandle_Invalid;
	if ( UsesColorAttribute )
		{
		ColorAttr = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Color ), sgValueType_Vec4 );
		if ( ColorAttr == sgNodeHandle_Invalid )
			return false;
		}

	sgNodeHandle White = sgAddConstantVec4 ( Graph, 1.0f, 1.0f, 1.0f, 1.0f );
	sgNodeHandle Diffuse = AddVec4Source ( Graph, Definition.DiffuseSource, MatTexType_Diffuse, UV, ColorAttr, White );
	if ( ( White == sgNodeHandle_Invalid ) || ( Diffuse == sgNodeHandle_Invalid ) )
		return false;

	sgNodeHandle FinalColor = Diffuse;
	if ( UseLighting )
		{
		sgNodeHandle WorldPosition3 = sgAddSwizzle ( Graph, "xyz", sgValueType_Vec4, sgValueType_Vec3 );
		sgNodeHandle Specular = AddVec4Source ( Graph, Definition.SpecularSource, MatTexType_Specular, UV, ColorAttr, Diffuse );
		sgNodeHandle ShininessAttr = sgNodeHandle_Invalid;
		if ( Definition.ShininessSource == scSource_Attribute )
			{
			ShininessAttr = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Shininess ), sgValueType_Float );
			if ( ShininessAttr == sgNodeHandle_Invalid )
				return false;
			}
		sgNodeHandle DefaultShininess = sgAddConstantFloat ( Graph, 0.6f );
		sgNodeHandle Shininess = AddFloatSource ( Graph, Definition.ShininessSource, MatTexType_Shininess, UV, ShininessAttr, DefaultShininess );
		sgNodeHandle NormalMatrix = sgAddUniform ( Graph, "u_NormalMatrix", sgValueType_Mat3 );
		sgNodeHandle Normal = AddWorldNormal ( Graph, Definition, NormalMatrix, ShadingNormal, ShadingNormalPin, UV );
		sgNodeHandle CameraPosition = sgAddUniform ( Graph, "u_ViewPosition", sgValueType_Vec3 );
		sgNodeHandle Lit = sgAddBlinnPhongNode ( Graph );
		if ( ( sgAddConnection ( Graph, WorldPosition4, 0, WorldPosition3, 0 ) == false ) ||
		        ( Specular == sgNodeHandle_Invalid ) || ( DefaultShininess == sgNodeHandle_Invalid ) ||
		        ( Shininess == sgNodeHandle_Invalid ) || ( NormalMatrix == sgNodeHandle_Invalid ) ||
		        ( Normal == sgNodeHandle_Invalid ) ||
		        ( CameraPosition == sgNodeHandle_Invalid ) ||
		        ( sgAddConnection ( Graph, Diffuse, 0, Lit, 0 ) == false ) ||
		        ( sgAddConnection ( Graph, Specular, 0, Lit, 1 ) == false ) ||
		        ( sgAddConnection ( Graph, Shininess, 0, Lit, 2 ) == false ) ||
		        ( sgAddConnection ( Graph, Normal, 0, Lit, 3 ) == false ) ||
		        ( sgAddConnection ( Graph, WorldPosition3, 0, Lit, 4 ) == false ) ||
		        ( sgAddConnection ( Graph, CameraPosition, 0, Lit, 5 ) == false ) )
			return false;
		FinalColor = Lit;

		if ( Definition.EmissivenessSource != scSource_Off )
			{
			sgNodeHandle EmissiveAttr = sgNodeHandle_Invalid;
			if ( Definition.EmissivenessSource == scSource_Attribute )
				{
				EmissiveAttr = sgAddAttribute ( Graph, scGetStandardAttributeName ( scAttribute_Emissiveness ), sgValueType_Vec4 );
				if ( EmissiveAttr == sgNodeHandle_Invalid )
					return false;
				}
			sgNodeHandle Black = sgAddConstantVec4 ( Graph, 0.0f, 0.0f, 0.0f, 0.0f );
			sgNodeHandle Emissive = AddVec4Source ( Graph, Definition.EmissivenessSource, MatTexType_Emissive, UV, EmissiveAttr, Black );
			sgNodeHandle Sum = sgAddAddition ( Graph );
			if ( ( Black == sgNodeHandle_Invalid ) || ( Emissive == sgNodeHandle_Invalid ) ||
			        ( sgAddConnection ( Graph, FinalColor, 0, Sum, 0 ) == false ) ||
			        ( sgAddConnection ( Graph, Emissive, 0, Sum, 1 ) == false ) )
				return false;
			FinalColor = Sum;
			}
		}

	sgNodeHandle FragmentColor = sgAddFragmentColor ( Graph );
	if ( sgAddConnection ( Graph, FinalColor, 0, FragmentColor, 0 ) == false )
		return false;
	return true;
	}

static sgGraphHandle BuildShaderGraph ( const scShaderDefinition Definition )
	{
	if ( ( Definition.Skinning ) && ( Definition.BoneCount == 0 ) )
		{
		LOG_ERROR ( "Invalid bone count for shader creator" );
		return sgGraphHandle_Invalid;
		}

	sgGraphHandle Graph = sgCreate ();
	if ( Graph == sgGraphHandle_Invalid )
		return sgGraphHandle_Invalid;
	if ( FillShaderGraph ( Graph, Definition ) == false )
		{
		sgDestroy ( Graph );
		return sgGraphHandle_Invalid;
		}
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

	sgGraphHandle Graph = BuildShaderGraph ( NewRequirements );
	if ( Graph == sgGraphHandle_Invalid )
		{
		LOG_ERROR ( "Unable to build shader graph from definition" );
		return;
		}
	if ( sgGenerateGLSL ( Graph, sgGetDefaultGLSLGeneratorOptions(), &OutputCode->VertexShader, &OutputCode->FragmentShader ) == false )
		{
		LOG_ERROR ( "Unable to generate GLSL from shader graph" );
		SAFE_DEL_C ( OutputCode->VertexShader );
		SAFE_DEL_C ( OutputCode->FragmentShader );
		SAFE_DEL_C ( OutputCode->GeometryShader );
		}
	sgDestroy ( Graph );
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
			STRINGIFY ( Shininess );
			STRINGIFY ( Emissiveness );
		case scAttribute_Count:
			break;
		}
#undef STRINGIFY
	return "Invalid scShaderAttributeType";
	}
