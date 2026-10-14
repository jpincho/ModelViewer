#if 0
#include "ShaderCreator.h"
#include <C_CrossRenderer/OpenGL4/GL4Renderer.h>
#include <Platform/ArrayUtils.h>
#include <Platform/Logger.h>
#include "StringBuilder.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define scNode_MaxInputs 8
#define scNode_MaxOutputs 4

typedef enum
	{
	scStage_Unknown = 0,
	scStage_Any,
	scStage_Vertex,
	scStage_Fragment
	} scStage;

typedef struct
	{
	scNodeType Type;
	char ResourceName[64];
	char VaryingName[32];
	char Swizzle[8];
	unsigned InputCount;
	scPin Inputs[scNode_MaxInputs];
	unsigned OutputCount;
	scValueType OutputTypes[scNode_MaxOutputs];
	unsigned ArrayCount;
	} scNode;

struct scShaderGraph
	{
	scNode *Nodes;
	unsigned NodeCount, NodeCapacity;
	scNodeId VertexPositionNode;
	scNodeId FragmentColorNode;
	};

typedef struct
	{
	scStage Stage;
	bool EmitVS;
	bool EmitFS;
	bool Interpolate;
	bool UsedVS;
	bool UsedFS;
	bool Visiting;
	bool EmittedVS;
	bool EmittedFS;
	} NodeState;

static const char LightUniformsCode[] =
    "struct AmbientLightData\n"
    "    {\n"
    "    vec3 Color;\n"
    "    float Intensity;\n"
    "    };\n"
    "\n"
    "struct PointLightData\n"
    "    {\n"
    "    vec4 Position;\n"
    "    vec4 Diffuse, Specular;\n"
    "    float Intensity;\n"
    "    float ConstantAttenuation;\n"
    "    float LinearAttenuation;\n"
    "    float ExponentialAttenuation;\n"
    "    };\n"
    "struct SpotLightData\n"
    "    {\n"
    "    vec4 Position;\n"
    "    vec4 Direction;\n"
    "    vec4 Diffuse;\n"
    "    vec4 Specular;\n"
    "    float Intensity;\n"
    "    float ConstantAttenuation;\n"
    "    float LinearAttenuation;\n"
    "    float ExponentialAttenuation;\n"
    "    float CutOffAngleCosine;\n"
    "    float CutOffOuterAngleCosine;\n"
    "    };\n"
    "struct DirectionalLightData\n"
    "    {\n"
    "    vec4 Direction;\n"
    "    vec4 Diffuse;\n"
    "    vec4 Specular;\n"
    "    float Intensity;\n"
    "    };\n"
    "layout ( std140 ) uniform u_LightData\n"
    "    {\n"
    "    AmbientLightData AmbientLight;\n"
    "    PointLightData PointLights[8];\n"
    "    SpotLightData SpotLights[8];\n"
    "    DirectionalLightData DirectionalLights[8];\n"
    "    int PointLightCount, SpotLightCount, DirectionalLightCount;\n"
    "    } LightData;\n";

static const char BlinnPhongLightingCode[] =
    "vec3 CalculatePointLightContribution ( int LightIndex, vec3 FragPosition, vec3 ViewPosToFrag, vec3 Normal, vec3 MaterialDiffuse, vec3 MaterialSpecular, float MaterialShininess )\n"
    "    {\n"
    "    vec3 LightPosToFrag = LightData.PointLights[LightIndex].Position.xyz - FragPosition;\n"
    "    float Distance = length ( LightPosToFrag );\n"
    "    float Attenuation = LightData.PointLights[LightIndex].ConstantAttenuation + LightData.PointLights[LightIndex].LinearAttenuation * Distance + LightData.PointLights[LightIndex].ExponentialAttenuation * pow ( Distance, 2.0 );\n"
    "    vec3 NormalizedLightToFrag = normalize ( LightPosToFrag );\n"
    "    float DiffuseComponent = max ( dot ( Normal, NormalizedLightToFrag ), 0.0 );\n"
    "    vec3 NormalizedViewPosToFrag = normalize ( ViewPosToFrag );\n"
    "    vec3 HalfwayDir = normalize ( NormalizedLightToFrag + NormalizedViewPosToFrag );\n"
    "    float SpecularComponent = pow ( max ( dot ( Normal, HalfwayDir ), 0.0 ), MaterialShininess );\n"
    "    vec3 DiffuseContribution = DiffuseComponent * LightData.PointLights[LightIndex].Diffuse.xyz * MaterialDiffuse * LightData.PointLights[LightIndex].Intensity;\n"
    "    vec3 SpecularContribution = SpecularComponent * LightData.PointLights[LightIndex].Specular.xyz * MaterialSpecular * LightData.PointLights[LightIndex].Intensity;\n"
    "    DiffuseContribution /= Attenuation;\n"
    "    SpecularContribution /= Attenuation;\n"
    "    return DiffuseContribution + SpecularContribution;\n"
    "    }\n"
    "\n"
    "vec3 CalculateSpotLightContribution ( int LightIndex, vec3 FragPosition, vec3 ViewPosToFrag, vec3 Normal, vec3 MaterialDiffuse, vec3 MaterialSpecular, float MaterialShininess )\n"
    "    {\n"
    "    vec3 LightPosToFrag = LightData.SpotLights[LightIndex].Position.xyz - FragPosition;\n"
    "    float Distance = length ( LightPosToFrag );\n"
    "    float Attenuation = LightData.SpotLights[LightIndex].ConstantAttenuation + LightData.SpotLights[LightIndex].LinearAttenuation * Distance + LightData.SpotLights[LightIndex].ExponentialAttenuation * pow ( Distance, 2.0 );\n"
    "    vec3 NormalizedLightToFrag = normalize ( LightPosToFrag );\n"
    "    float DiffuseComponent = max ( dot ( Normal, NormalizedLightToFrag ), 0.0 );\n"
    "    if ( DiffuseComponent == 0.0 )\n"
    "        return vec3 ( 0.0 );\n"
    "    vec3 NormalizedViewPosToFrag = normalize ( ViewPosToFrag );\n"
    "    vec3 HalfwayDir = normalize ( NormalizedLightToFrag + NormalizedViewPosToFrag );\n"
    "    float SpecularComponent = pow ( max ( dot ( Normal, HalfwayDir ), 0.0 ), MaterialShininess );\n"
    "    float theta = dot ( NormalizedLightToFrag, normalize ( -LightData.SpotLights[LightIndex].Direction.xyz ) );\n"
    "    float epsilon = LightData.SpotLights[LightIndex].CutOffAngleCosine - LightData.SpotLights[LightIndex].CutOffOuterAngleCosine;\n"
    "    float intensity = clamp ( ( theta - LightData.SpotLights[LightIndex].CutOffOuterAngleCosine ) / epsilon, 0.0, 1.0 );\n"
    "    if ( intensity == 0.0 )\n"
    "        return vec3 ( 0.0 );\n"
    "    vec3 DiffuseContribution = DiffuseComponent * LightData.SpotLights[LightIndex].Diffuse.xyz * MaterialDiffuse * LightData.SpotLights[LightIndex].Intensity;\n"
    "    vec3 SpecularContribution = SpecularComponent * LightData.SpotLights[LightIndex].Specular.xyz * MaterialSpecular * LightData.SpotLights[LightIndex].Intensity;\n"
    "    DiffuseContribution /= Attenuation;\n"
    "    SpecularContribution /= Attenuation;\n"
    "    return ( DiffuseContribution + SpecularContribution ) * intensity;\n"
    "    }\n"
    "\n"
    "vec3 CalculateDirectionalLightContribution ( int LightIndex, vec3 ViewPosToFrag, vec3 Normal, vec3 MaterialDiffuse, vec3 MaterialSpecular, float MaterialShininess )\n"
    "    {\n"
    "    vec3 NormalizedLightDir = normalize ( -LightData.DirectionalLights[LightIndex].Direction.xyz );\n"
    "    float DiffuseComponent = max ( dot ( Normal, NormalizedLightDir ), 0.0 );\n"
    "    vec3 NormalizedViewPosToFrag = normalize ( ViewPosToFrag );\n"
    "    vec3 HalfwayDir = normalize ( NormalizedLightDir + NormalizedViewPosToFrag );\n"
    "    float SpecularComponent = pow ( max ( dot ( Normal, HalfwayDir ), 0.0 ), MaterialShininess );\n"
    "    vec3 DiffuseContribution = DiffuseComponent * LightData.DirectionalLights[LightIndex].Diffuse.xyz * MaterialDiffuse * LightData.DirectionalLights[LightIndex].Intensity;\n"
    "    vec3 SpecularContribution = SpecularComponent * LightData.DirectionalLights[LightIndex].Specular.xyz * MaterialSpecular * LightData.DirectionalLights[LightIndex].Intensity;\n"
    "    return DiffuseContribution + SpecularContribution;\n"
    "    }\n"
    "\n"
    "vec4 ApplyLighting ( vec4 MaterialDiffuse, vec4 MaterialSpecular, float MaterialShininess, vec3 Normal, vec3 FragPosition, vec3 ViewPosition )\n"
    "    {\n"
    "    vec3 Output = vec3 ( 0.0, 0.0, 0.0 );\n"
    "    vec3 TotalPointLightContribution = vec3 ( 0.0, 0.0, 0.0 );\n"
    "    vec3 TotalSpotLightContribution = vec3 ( 0.0, 0.0, 0.0 );\n"
    "    vec3 TotalDirectionalLightContribution = vec3 ( 0.0, 0.0, 0.0 );\n"
    "    vec3 ViewToFrag = ViewPosition - FragPosition;\n"
    "    for ( int PointLightIndex = 0; PointLightIndex < LightData.PointLightCount; ++PointLightIndex )\n"
    "        TotalPointLightContribution += CalculatePointLightContribution ( PointLightIndex, FragPosition, ViewToFrag, Normal, MaterialDiffuse.xyz, MaterialSpecular.xyz, MaterialShininess );\n"
    "    for ( int SpotLightIndex = 0; SpotLightIndex < LightData.SpotLightCount; ++SpotLightIndex )\n"
    "        TotalSpotLightContribution += CalculateSpotLightContribution ( SpotLightIndex, FragPosition, ViewToFrag, Normal, MaterialDiffuse.xyz, MaterialSpecular.xyz, MaterialShininess );\n"
    "    for ( int DirectionalLightIndex = 0; DirectionalLightIndex < LightData.DirectionalLightCount; ++DirectionalLightIndex )\n"
    "        TotalDirectionalLightContribution += CalculateDirectionalLightContribution ( DirectionalLightIndex, ViewToFrag, Normal, MaterialDiffuse.xyz, MaterialSpecular.xyz, MaterialShininess );\n"
    "    Output += TotalPointLightContribution;\n"
    "    Output += TotalSpotLightContribution;\n"
    "    Output += TotalDirectionalLightContribution;\n"
    "    vec4 AmbientLightContribution = vec4 ( LightData.AmbientLight.Color, 1.0 ) * LightData.AmbientLight.Intensity * MaterialDiffuse;\n"
    "    Output += AmbientLightContribution.xyz;\n"
    "    return vec4 ( Output, MaterialDiffuse.a );\n"
    "    }\n";

const char *Stringify_scValueType ( const scValueType Type )
	{
	switch ( Type )
		{
		case scValue_Float:
			return "float";
		case scValue_Vec2:
			return "vec2";
		case scValue_Vec3:
			return "vec3";
		case scValue_Vec4:
			return "vec4";
		case scValue_UVec4:
			return "uvec4";
		case scValue_Mat3:
			return "mat3";
		case scValue_Mat4:
			return "mat4";
		case scValue_Sampler2D:
			return "sampler2D";
		case scValue_Void:
			return "void";
		}
	return "unknown";
	}

const char *Stringify_scNodeType ( const scNodeType Type )
	{
	switch ( Type )
		{
		case scNodeType_Attribute:
			return "Attribute";
		case scNodeType_Uniform:
			return "Uniform";
		case scNodeType_Constant:
			return "Constant";
		case scNodeType_TextureSample:
			return "TextureSample";
		case scNodeType_Multiply:
			return "Multiply";
		case scNodeType_Add:
			return "Add";
		case scNodeType_Normalize:
			return "Normalize";
		case scNodeType_ComposeVec4:
			return "ComposeVec4";
		case scNodeType_Swizzle:
			return "Swizzle";
		case scNodeType_Mat3Cast:
			return "Mat3Cast";
		case scNodeType_Skinning:
			return "Skinning";
		case scNodeType_TBN:
			return "TBN";
		case scNodeType_NormalMap:
			return "NormalMap";
		case scNodeType_BlinnPhong:
			return "BlinnPhong";
		case scNodeType_VertexPosition:
			return "VertexPosition";
		case scNodeType_FragmentColor:
			return "FragmentColor";
		}
	return "unknown";
	}

static unsigned EncodeGLSLVersion ( const VersionNumber Version )
	{
	unsigned Minor = Version.Minor;
	if ( Minor < 10 )
		Minor *= 10;
	const unsigned Encoded = Version.Major * 100 + Minor;
	if ( Encoded == 0 )
		return 400;
	return Encoded;
	}

static bool IsValidNode ( const scShaderGraph *Graph, const scNodeId Node )
	{
	return ( Graph != NULL ) && ( Node != scNodeId_Invalid ) && ( Node < Graph->NodeCount );
	}

static bool IsValidPin ( const scShaderGraph *Graph, const scPin Pin )
	{
	if ( IsValidNode ( Graph, Pin.Node ) == false )
		return false;
	return ( Pin.Output < Graph->Nodes[Pin.Node].OutputCount );
	}

static scValueType GetPinType ( const scShaderGraph *Graph, const scPin Pin )
	{
	if ( IsValidPin ( Graph, Pin ) == false )
		return scValue_Void;
	return Graph->Nodes[Pin.Node].OutputTypes[Pin.Output];
	}

static bool IsVertexBoundary ( const scNodeType Type )
	{
	return ( Type == scNodeType_Attribute ) ||
	       ( Type == scNodeType_Skinning ) ||
	       ( Type == scNodeType_TBN ) ||
	       ( Type == scNodeType_VertexPosition );
	}

static bool IsFragmentForced ( const scNodeType Type )
	{
	return ( Type == scNodeType_TextureSample ) ||
	       ( Type == scNodeType_NormalMap ) ||
	       ( Type == scNodeType_BlinnPhong ) ||
	       ( Type == scNodeType_FragmentColor );
	}

static bool IsLeafValue ( const scNodeType Type )
	{
	return ( Type == scNodeType_Attribute ) ||
	       ( Type == scNodeType_Uniform ) ||
	       ( Type == scNodeType_Constant );
	}

static scValueType MultiplyOutputType ( const scValueType A, const scValueType B )
	{
	if ( ( A == scValue_Mat4 ) && ( B == scValue_Mat4 ) )
		return scValue_Mat4;
	if ( ( A == scValue_Mat4 ) && ( B == scValue_Vec4 ) )
		return scValue_Vec4;
	if ( ( A == scValue_Mat3 ) && ( B == scValue_Vec3 ) )
		return scValue_Vec3;
	if ( ( A == scValue_Float ) && ( B != scValue_Void ) )
		return B;
	if ( ( B == scValue_Float ) && ( A != scValue_Void ) )
		return A;
	if ( A == B )
		return A;
	return scValue_Void;
	}

static scValueType SwizzleOutputType ( const char *Swizzle )
	{
	if ( Swizzle == NULL )
		return scValue_Void;
	switch ( strlen ( Swizzle ) )
		{
		case 1:
			return scValue_Float;
		case 2:
			return scValue_Vec2;
		case 3:
			return scValue_Vec3;
		case 4:
			return scValue_Vec4;
		default:
			return scValue_Void;
		}
	}

static scNode MakeEmptyNode ( const scNodeType Type )
	{
	scNode Node = {0};
	Node.Type = Type;
	return Node;
	}

static scNodeId PushNode ( scShaderGraph *Graph, const scNode Node )
	{
	ARRAY_PUSH_BACK ( Graph->Nodes, Graph->NodeCount, Graph->NodeCapacity, Node );
	return Graph->NodeCount - 1;
	}

static scNodeId AddBinary ( scShaderGraph *Graph, const scNodeType Type, const scPin A, const scPin B, const scValueType OutputType )
	{
	if ( ( IsValidPin ( Graph, A ) == false ) || ( IsValidPin ( Graph, B ) == false ) || ( OutputType == scValue_Void ) )
		{
		LOG_ERROR ( "Invalid inputs for %s node", Stringify_scNodeType ( Type ) );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( Type );
	Node.InputCount = 2;
	Node.Inputs[0] = A;
	Node.Inputs[1] = B;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = OutputType;
	return PushNode ( Graph, Node );
	}

scShaderGraph *scGraph_Create ( void )
	{
	scShaderGraph *Graph = calloc ( 1, sizeof ( scShaderGraph ) );
	if ( Graph == NULL )
		return NULL;
	Graph->VertexPositionNode = scNodeId_Invalid;
	Graph->FragmentColorNode = scNodeId_Invalid;
	return Graph;
	}

void scGraph_Destroy ( scShaderGraph *Graph )
	{
	if ( Graph == NULL )
		return;
	Array_Free ( ( void ** ) &Graph->Nodes, &Graph->NodeCount, &Graph->NodeCapacity );
	free ( Graph );
	}

scNodeId scGraph_AddAttribute ( scShaderGraph *Graph, const char *Name, const scValueType Type )
	{
	if ( ( Graph == NULL ) || ( Name == NULL ) || ( Type == scValue_Void ) )
		return scNodeId_Invalid;
	scNode Node = MakeEmptyNode ( scNodeType_Attribute );
	strncpy ( Node.ResourceName, Name, sizeof ( Node.ResourceName ) );
	Node.OutputCount = 1;
	Node.OutputTypes[0] = Type;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddUniform ( scShaderGraph *Graph, const char *Name, const scValueType Type )
	{
	if ( ( Graph == NULL ) || ( Name == NULL ) || ( Type == scValue_Void ) )
		return scNodeId_Invalid;
	scNode Node = MakeEmptyNode ( scNodeType_Uniform );
	strncpy ( Node.ResourceName, Name, sizeof ( Node.ResourceName ) );
	Node.OutputCount = 1;
	Node.OutputTypes[0] = Type;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddUniformArray ( scShaderGraph *Graph, const char *Name, const scValueType Type, const unsigned Count )
	{
	if ( ( Graph == NULL ) || ( Name == NULL ) || ( Type == scValue_Void ) || ( Count == 0 ) )
		return scNodeId_Invalid;
	scNode Node = MakeEmptyNode ( scNodeType_Uniform );
	strncpy ( Node.ResourceName, Name, sizeof ( Node.ResourceName ) );
	Node.OutputCount = 1;
	Node.OutputTypes[0] = Type;
	Node.ArrayCount = Count;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddConstantFloat ( scShaderGraph *Graph, const float Value )
	{
	if ( Graph == NULL )
		return scNodeId_Invalid;
	scNode Node = MakeEmptyNode ( scNodeType_Constant );
	snprintf ( Node.ResourceName, sizeof ( Node.ResourceName ), "%g", Value );
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Float;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddConstantVec3 ( scShaderGraph *Graph, const float X, const float Y, const float Z )
	{
	if ( Graph == NULL )
		return scNodeId_Invalid;
	scNode Node = MakeEmptyNode ( scNodeType_Constant );
	snprintf ( Node.ResourceName, sizeof ( Node.ResourceName ), "vec3 ( %g, %g, %g )", X, Y, Z );
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Vec3;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddConstantVec4 ( scShaderGraph *Graph, const float X, const float Y, const float Z, const float W )
	{
	if ( Graph == NULL )
		return scNodeId_Invalid;
	scNode Node = MakeEmptyNode ( scNodeType_Constant );
	snprintf ( Node.ResourceName, sizeof ( Node.ResourceName ), "vec4 ( %g, %g, %g, %g )", X, Y, Z, W );
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Vec4;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddIdentityMat4 ( scShaderGraph *Graph )
	{
	if ( Graph == NULL )
		return scNodeId_Invalid;
	scNode Node = MakeEmptyNode ( scNodeType_Constant );
	strncpy ( Node.ResourceName, "mat4 ( 1.0 )", sizeof ( Node.ResourceName ) );
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Mat4;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddTextureSample ( scShaderGraph *Graph, const scPin Sampler, const scPin UV )
	{
	if ( ( GetPinType ( Graph, Sampler ) != scValue_Sampler2D ) || ( GetPinType ( Graph, UV ) != scValue_Vec2 ) )
		{
		LOG_ERROR ( "TextureSample requires sampler2D and vec2 UV" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_TextureSample );
	Node.InputCount = 2;
	Node.Inputs[0] = Sampler;
	Node.Inputs[1] = UV;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Vec4;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddMultiply ( scShaderGraph *Graph, const scPin A, const scPin B )
	{
	return AddBinary ( Graph, scNodeType_Multiply, A, B, MultiplyOutputType ( GetPinType ( Graph, A ), GetPinType ( Graph, B ) ) );
	}

scNodeId scGraph_AddAdd ( scShaderGraph *Graph, const scPin A, const scPin B )
	{
	const scValueType TypeA = GetPinType ( Graph, A );
	const scValueType TypeB = GetPinType ( Graph, B );
	if ( TypeA != TypeB )
		{
		LOG_ERROR ( "Add requires matching input types" );
		return scNodeId_Invalid;
		}
	return AddBinary ( Graph, scNodeType_Add, A, B, TypeA );
	}

scNodeId scGraph_AddNormalize ( scShaderGraph *Graph, const scPin Value )
	{
	const scValueType Type = GetPinType ( Graph, Value );
	if ( ( Type != scValue_Vec2 ) && ( Type != scValue_Vec3 ) && ( Type != scValue_Vec4 ) )
		{
		LOG_ERROR ( "Normalize requires a vector" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_Normalize );
	Node.InputCount = 1;
	Node.Inputs[0] = Value;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = Type;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddComposeVec4 ( scShaderGraph *Graph, const scPin XYZ, const scPin W )
	{
	if ( ( GetPinType ( Graph, XYZ ) != scValue_Vec3 ) || ( GetPinType ( Graph, W ) != scValue_Float ) )
		{
		LOG_ERROR ( "ComposeVec4 requires vec3 and float" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_ComposeVec4 );
	Node.InputCount = 2;
	Node.Inputs[0] = XYZ;
	Node.Inputs[1] = W;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Vec4;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddSwizzle ( scShaderGraph *Graph, const scPin Value, const char *Swizzle )
	{
	const scValueType OutputType = SwizzleOutputType ( Swizzle );
	if ( ( IsValidPin ( Graph, Value ) == false ) || ( OutputType == scValue_Void ) )
		{
		LOG_ERROR ( "Invalid swizzle" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_Swizzle );
	strncpy ( Node.Swizzle, Swizzle, sizeof ( Node.Swizzle ) );
	Node.InputCount = 1;
	Node.Inputs[0] = Value;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = OutputType;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddMat3Cast ( scShaderGraph *Graph, const scPin Matrix4 )
	{
	if ( GetPinType ( Graph, Matrix4 ) != scValue_Mat4 )
		{
		LOG_ERROR ( "Mat3Cast requires mat4" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_Mat3Cast );
	Node.InputCount = 1;
	Node.Inputs[0] = Matrix4;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Mat3;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddSkinning ( scShaderGraph *Graph, const scPin Position, const scPin BoneIndex, const scPin BoneWeight, const scPin Bones )
	{
	if ( ( GetPinType ( Graph, Position ) != scValue_Vec3 ) ||
	        ( GetPinType ( Graph, BoneIndex ) != scValue_UVec4 ) ||
	        ( GetPinType ( Graph, BoneWeight ) != scValue_Vec4 ) ||
	        ( GetPinType ( Graph, Bones ) != scValue_Mat4 ) )
		{
		LOG_ERROR ( "Skinning requires vec3 position, uvec4 indices, vec4 weights, and mat4 bones" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_Skinning );
	Node.InputCount = 4;
	Node.Inputs[0] = Position;
	Node.Inputs[1] = BoneIndex;
	Node.Inputs[2] = BoneWeight;
	Node.Inputs[3] = Bones;
	Node.OutputCount = 2;
	Node.OutputTypes[scSkinningOut_BoneMatrix] = scValue_Mat4;
	Node.OutputTypes[scSkinningOut_Position] = scValue_Vec4;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddTBN ( scShaderGraph *Graph, const scPin Tangent, const scPin Bitangent, const scPin Normal, const scPin ModelMatrix, const scPin BoneMatrix )
	{
	if ( ( GetPinType ( Graph, Tangent ) != scValue_Vec3 ) ||
	        ( GetPinType ( Graph, Bitangent ) != scValue_Vec3 ) ||
	        ( GetPinType ( Graph, Normal ) != scValue_Vec3 ) ||
	        ( GetPinType ( Graph, ModelMatrix ) != scValue_Mat4 ) ||
	        ( GetPinType ( Graph, BoneMatrix ) != scValue_Mat4 ) )
		{
		LOG_ERROR ( "TBN requires vec3 tangent/bitangent/normal and mat4 model/bone matrices" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_TBN );
	Node.InputCount = 5;
	Node.Inputs[0] = Tangent;
	Node.Inputs[1] = Bitangent;
	Node.Inputs[2] = Normal;
	Node.Inputs[3] = ModelMatrix;
	Node.Inputs[4] = BoneMatrix;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Mat3;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddNormalMap ( scShaderGraph *Graph, const scPin Sampler, const scPin UV, const scPin TBN )
	{
	if ( ( GetPinType ( Graph, Sampler ) != scValue_Sampler2D ) ||
	        ( GetPinType ( Graph, UV ) != scValue_Vec2 ) ||
	        ( GetPinType ( Graph, TBN ) != scValue_Mat3 ) )
		{
		LOG_ERROR ( "NormalMap requires sampler2D, vec2 UV, and mat3 TBN" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_NormalMap );
	Node.InputCount = 3;
	Node.Inputs[0] = Sampler;
	Node.Inputs[1] = UV;
	Node.Inputs[2] = TBN;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Vec3;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddBlinnPhong ( scShaderGraph *Graph, const scPin Diffuse, const scPin Specular, const scPin Shininess, const scPin Normal, const scPin WorldPosition, const scPin ViewPosition )
	{
	if ( ( GetPinType ( Graph, Diffuse ) != scValue_Vec4 ) ||
	        ( GetPinType ( Graph, Specular ) != scValue_Vec4 ) ||
	        ( GetPinType ( Graph, Shininess ) != scValue_Float ) ||
	        ( GetPinType ( Graph, Normal ) != scValue_Vec3 ) ||
	        ( GetPinType ( Graph, WorldPosition ) != scValue_Vec3 ) ||
	        ( GetPinType ( Graph, ViewPosition ) != scValue_Vec3 ) )
		{
		LOG_ERROR ( "BlinnPhong requires vec4 diffuse/specular, float shininess, and vec3 normal/world/view" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_BlinnPhong );
	Node.InputCount = 6;
	Node.Inputs[0] = Diffuse;
	Node.Inputs[1] = Specular;
	Node.Inputs[2] = Shininess;
	Node.Inputs[3] = Normal;
	Node.Inputs[4] = WorldPosition;
	Node.Inputs[5] = ViewPosition;
	Node.OutputCount = 1;
	Node.OutputTypes[0] = scValue_Vec4;
	return PushNode ( Graph, Node );
	}

scNodeId scGraph_AddVertexPosition ( scShaderGraph *Graph, const scPin ClipPosition )
	{
	if ( GetPinType ( Graph, ClipPosition ) != scValue_Vec4 )
		{
		LOG_ERROR ( "VertexPosition requires vec4 clip position" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_VertexPosition );
	Node.InputCount = 1;
	Node.Inputs[0] = ClipPosition;
	Graph->VertexPositionNode = PushNode ( Graph, Node );
	return Graph->VertexPositionNode;
	}

scNodeId scGraph_AddFragmentColor ( scShaderGraph *Graph, const scPin Color )
	{
	if ( GetPinType ( Graph, Color ) != scValue_Vec4 )
		{
		LOG_ERROR ( "FragmentColor requires vec4" );
		return scNodeId_Invalid;
		}
	scNode Node = MakeEmptyNode ( scNodeType_FragmentColor );
	Node.InputCount = 1;
	Node.Inputs[0] = Color;
	Graph->FragmentColorNode = PushNode ( Graph, Node );
	return Graph->FragmentColorNode;
	}

bool scGraph_SetVaryingName ( scShaderGraph *Graph, const scNodeId Node, const char *Name )
	{
	if ( ( IsValidNode ( Graph, Node ) == false ) || ( Name == NULL ) )
		return false;
	strncpy ( Graph->Nodes[Node].VaryingName, Name, sizeof ( Graph->Nodes[Node].VaryingName ) );
	return true;
	}

static const char *VaryingNameOf ( const scNode *Node, const scNodeId Id, char *Buffer, const unsigned BufferSize )
	{
	if ( Node->VaryingName[0] != 0 )
		return Node->VaryingName;
	snprintf ( Buffer, BufferSize, "v_n%u", Id );
	return Buffer;
	}

static const char *LocalNameOf ( const scNode *Node, const scNodeId Id, const unsigned Output, char *Buffer, const unsigned BufferSize )
	{
	if ( Node->OutputCount <= 1 )
		snprintf ( Buffer, BufferSize, "n%u", Id );
	else
		snprintf ( Buffer, BufferSize, "n%u_%u", Id, Output );
	return Buffer;
	}

static const char *PinExpression ( const scShaderGraph *Graph, const NodeState *State, const scPin Pin, const scStage Stage, char *Buffer, const unsigned BufferSize )
	{
	if ( IsValidPin ( Graph, Pin ) == false )
		{
		strncpy ( Buffer, "vec4(0.0)", BufferSize );
		return Buffer;
		}
	const scNode *Node = &Graph->Nodes[Pin.Node];
	if ( ( Node->Type == scNodeType_Uniform ) || ( Node->Type == scNodeType_Constant ) )
		return Node->ResourceName;
	if ( Node->Type == scNodeType_Attribute )
		{
		if ( Stage == scStage_Fragment )
			return VaryingNameOf ( Node, Pin.Node, Buffer, BufferSize );
		return Node->ResourceName;
		}
	if ( ( Stage == scStage_Fragment ) && State[Pin.Node].Interpolate )
		return VaryingNameOf ( Node, Pin.Node, Buffer, BufferSize );
	return LocalNameOf ( Node, Pin.Node, Pin.Output, Buffer, BufferSize );
	}

static scStage ComputeStage ( const scShaderGraph *Graph, NodeState *State, const scNodeId Id )
	{
	if ( IsValidNode ( Graph, Id ) == false )
		return scStage_Any;
	if ( State[Id].Stage != scStage_Unknown )
		return State[Id].Stage;
	if ( State[Id].Visiting )
		{
		LOG_ERROR ( "Cycle detected in shader graph at node %u", Id );
		State[Id].Stage = scStage_Any;
		return scStage_Any;
		}

	const scNode *Node = &Graph->Nodes[Id];
	if ( ( Node->Type == scNodeType_Uniform ) || ( Node->Type == scNodeType_Constant ) )
		{
		State[Id].Stage = scStage_Any;
		return scStage_Any;
		}
	if ( IsVertexBoundary ( Node->Type ) )
		{
		State[Id].Stage = scStage_Vertex;
		return scStage_Vertex;
		}
	if ( IsFragmentForced ( Node->Type ) )
		{
		State[Id].Stage = scStage_Fragment;
		return scStage_Fragment;
		}

	State[Id].Visiting = true;
	bool HasVertex = false;
	bool HasFragment = false;
	unsigned Index;
	for ( Index = 0; Index < Node->InputCount; ++Index )
		{
		const scStage InputStage = ComputeStage ( Graph, State, Node->Inputs[Index].Node );
		if ( InputStage == scStage_Vertex )
			HasVertex = true;
		if ( InputStage == scStage_Fragment )
			HasFragment = true;
		}
	State[Id].Visiting = false;

	if ( HasFragment )
		State[Id].Stage = scStage_Fragment;
	else if ( HasVertex )
		State[Id].Stage = scStage_Vertex;
	else
		State[Id].Stage = scStage_Any;
	return State[Id].Stage;
	}

static void Promote ( const scShaderGraph *Graph, NodeState *State, const scNodeId Id, const scStage Required )
	{
	if ( IsValidNode ( Graph, Id ) == false )
		return;
	const scNode *Node = &Graph->Nodes[Id];
	NodeState *Current = &State[Id];

	if ( ( Node->Type == scNodeType_Uniform ) || ( Node->Type == scNodeType_Constant ) )
		{
		if ( Required == scStage_Vertex )
			Current->UsedVS = true;
		else
			Current->UsedFS = true;
		return;
		}

	ComputeStage ( Graph, State, Id );

	if ( ( Required == scStage_Fragment ) && ( Current->Stage == scStage_Vertex ) )
		{
		Current->Interpolate = true;
		Promote ( Graph, State, Id, scStage_Vertex );
		return;
		}

	if ( Current->Stage == scStage_Any )
		Current->Stage = Required;

	if ( Required == scStage_Vertex )
		{
		if ( Current->EmitVS )
			return;
		Current->EmitVS = true;
		Current->UsedVS = true;
		}
	else
		{
		if ( Current->EmitFS )
			return;
		Current->EmitFS = true;
		Current->UsedFS = true;
		}

	const scStage ChildRequired = ( Current->Stage == scStage_Any ) ? Required : Current->Stage;
	unsigned Index;
	for ( Index = 0; Index < Node->InputCount; ++Index )
		Promote ( Graph, State, Node->Inputs[Index].Node, ChildRequired );
	}

static bool EmitNode ( const scShaderGraph *Graph, NodeState *State, StringBuilder *Builder, const scNodeId Id, const scStage Stage )
	{
	if ( IsValidNode ( Graph, Id ) == false )
		return true;
	const scNode *Node = &Graph->Nodes[Id];
	NodeState *Current = &State[Id];

	if ( IsLeafValue ( Node->Type ) )
		return true;
	if ( ( Stage == scStage_Fragment ) && Current->Interpolate )
		return true;
	if ( ( Stage == scStage_Vertex ) && Current->EmittedVS )
		return true;
	if ( ( Stage == scStage_Fragment ) && Current->EmittedFS )
		return true;
	if ( ( Node->Type == scNodeType_VertexPosition ) || ( Node->Type == scNodeType_FragmentColor ) )
		return true;

	if ( Current->Visiting )
		{
		LOG_ERROR ( "Cycle detected while emitting node %u", Id );
		return false;
		}
	Current->Visiting = true;

	unsigned Index;
	for ( Index = 0; Index < Node->InputCount; ++Index )
		{
		if ( EmitNode ( Graph, State, Builder, Node->Inputs[Index].Node, Stage ) == false )
			{
			Current->Visiting = false;
			return false;
			}
		}

	char A[96], B[96], C[96], D[96], E[96], F[96];
	char Local[64];
	const char *TypeName = ( Node->OutputCount > 0 ) ? Stringify_scValueType ( Node->OutputTypes[0] ) : "void";
	LocalNameOf ( Node, Id, 0, Local, sizeof ( Local ) );

	switch ( Node->Type )
		{
		case scNodeType_TextureSample:
			StringBuilder_Appendf ( Builder, "%s %s = texture ( %s, %s );\n", TypeName, Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ),
			                        PinExpression ( Graph, State, Node->Inputs[1], Stage, B, sizeof ( B ) ) );
			break;
		case scNodeType_Multiply:
			StringBuilder_Appendf ( Builder, "%s %s = %s * %s;\n", TypeName, Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ),
			                        PinExpression ( Graph, State, Node->Inputs[1], Stage, B, sizeof ( B ) ) );
			break;
		case scNodeType_Add:
			StringBuilder_Appendf ( Builder, "%s %s = %s + %s;\n", TypeName, Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ),
			                        PinExpression ( Graph, State, Node->Inputs[1], Stage, B, sizeof ( B ) ) );
			break;
		case scNodeType_Normalize:
			StringBuilder_Appendf ( Builder, "%s %s = normalize ( %s );\n", TypeName, Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ) );
			break;
		case scNodeType_ComposeVec4:
			StringBuilder_Appendf ( Builder, "vec4 %s = vec4 ( %s, %s );\n", Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ),
			                        PinExpression ( Graph, State, Node->Inputs[1], Stage, B, sizeof ( B ) ) );
			break;
		case scNodeType_Swizzle:
			StringBuilder_Appendf ( Builder, "%s %s = %s.%s;\n", TypeName, Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ),
			                        Node->Swizzle );
			break;
		case scNodeType_Mat3Cast:
			StringBuilder_Appendf ( Builder, "mat3 %s = mat3 ( %s );\n", Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ) );
			break;
		case scNodeType_Skinning:
			{
			char BoneMatrixName[64];
			char PositionName[64];
			LocalNameOf ( Node, Id, scSkinningOut_BoneMatrix, BoneMatrixName, sizeof ( BoneMatrixName ) );
			LocalNameOf ( Node, Id, scSkinningOut_Position, PositionName, sizeof ( PositionName ) );
			const char *Indices = PinExpression ( Graph, State, Node->Inputs[1], Stage, A, sizeof ( A ) );
			const char *Weights = PinExpression ( Graph, State, Node->Inputs[2], Stage, B, sizeof ( B ) );
			const char *Bones = PinExpression ( Graph, State, Node->Inputs[3], Stage, C, sizeof ( C ) );
			const char *Position = PinExpression ( Graph, State, Node->Inputs[0], Stage, D, sizeof ( D ) );
			StringBuilder_Appendf ( Builder, "mat4 %s = %s[%s[0]] * %s[0];\n", BoneMatrixName, Bones, Indices, Weights );
			StringBuilder_Appendf ( Builder, "%s += %s[%s[1]] * %s[1];\n", BoneMatrixName, Bones, Indices, Weights );
			StringBuilder_Appendf ( Builder, "%s += %s[%s[2]] * %s[2];\n", BoneMatrixName, Bones, Indices, Weights );
			StringBuilder_Appendf ( Builder, "%s += %s[%s[3]] * %s[3];\n", BoneMatrixName, Bones, Indices, Weights );
			StringBuilder_Appendf ( Builder, "vec4 %s = %s * vec4 ( %s, 1.0 );\n", PositionName, BoneMatrixName, Position );
			break;
			}
		case scNodeType_TBN:
			{
			const char *Tangent = PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) );
			const char *Bitangent = PinExpression ( Graph, State, Node->Inputs[1], Stage, B, sizeof ( B ) );
			const char *Normal = PinExpression ( Graph, State, Node->Inputs[2], Stage, C, sizeof ( C ) );
			const char *Model = PinExpression ( Graph, State, Node->Inputs[3], Stage, D, sizeof ( D ) );
			const char *Bone = PinExpression ( Graph, State, Node->Inputs[4], Stage, E, sizeof ( E ) );
			StringBuilder_Appendf ( Builder, "vec3 %s_T = normalize ( vec3 ( %s * %s * vec4 ( %s, 0.0 ) ) );\n", Local, Model, Bone, Tangent );
			StringBuilder_Appendf ( Builder, "vec3 %s_B = normalize ( vec3 ( %s * %s * vec4 ( %s, 0.0 ) ) );\n", Local, Model, Bone, Bitangent );
			StringBuilder_Appendf ( Builder, "vec3 %s_N = normalize ( vec3 ( %s * %s * vec4 ( %s, 0.0 ) ) );\n", Local, Model, Bone, Normal );
			StringBuilder_Appendf ( Builder, "mat3 %s = mat3 ( %s_T, %s_B, %s_N );\n", Local, Local, Local, Local );
			break;
			}
		case scNodeType_NormalMap:
			StringBuilder_Appendf ( Builder, "vec3 %s = texture ( %s, %s ).rgb * 2.0 - 1.0;\n", Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ),
			                        PinExpression ( Graph, State, Node->Inputs[1], Stage, B, sizeof ( B ) ) );
			StringBuilder_Appendf ( Builder, "%s = normalize ( %s * %s );\n", Local,
			                        PinExpression ( Graph, State, Node->Inputs[2], Stage, C, sizeof ( C ) ),
			                        Local );
			break;
		case scNodeType_BlinnPhong:
			StringBuilder_Appendf ( Builder, "vec4 %s = ApplyLighting ( %s, %s, %s, %s, %s, %s );\n", Local,
			                        PinExpression ( Graph, State, Node->Inputs[0], Stage, A, sizeof ( A ) ),
			                        PinExpression ( Graph, State, Node->Inputs[1], Stage, B, sizeof ( B ) ),
			                        PinExpression ( Graph, State, Node->Inputs[2], Stage, C, sizeof ( C ) ),
			                        PinExpression ( Graph, State, Node->Inputs[3], Stage, D, sizeof ( D ) ),
			                        PinExpression ( Graph, State, Node->Inputs[4], Stage, E, sizeof ( E ) ),
			                        PinExpression ( Graph, State, Node->Inputs[5], Stage, F, sizeof ( F ) ) );
			break;
		default:
			LOG_ERROR ( "Unhandled node type %s", Stringify_scNodeType ( Node->Type ) );
			Current->Visiting = false;
			return false;
		}

	Current->Visiting = false;
	if ( Stage == scStage_Vertex )
		Current->EmittedVS = true;
	else
		Current->EmittedFS = true;
	return true;
	}

static bool GraphHasType ( const scShaderGraph *Graph, const NodeState *State, const scNodeType Type, const bool Fragment )
	{
	unsigned Index;
	for ( Index = 0; Index < Graph->NodeCount; ++Index )
		{
		if ( Graph->Nodes[Index].Type != Type )
			continue;
		if ( Fragment && State[Index].EmitFS )
			return true;
		if ( ( Fragment == false ) && State[Index].EmitVS )
			return true;
		}
	return false;
	}

bool scGraph_GenerateCode ( scShaderGraph *Graph, crShaderCode *OutputCode )
	{
	OutputCode->VertexShader = NULL;
	OutputCode->FragmentShader = NULL;
	OutputCode->GeometryShader = NULL;
	if ( Graph == NULL )
		{
		LOG_ERROR ( "Shader graph not present" );
		return false;
		}
	if ( Graph->VertexPositionNode == scNodeId_Invalid )
		{
		LOG_ERROR ( "Shader graph is missing vertex position output" );
		return false;
		}
	if ( Graph->FragmentColorNode == scNodeId_Invalid )
		{
		LOG_ERROR ( "Shader graph is missing fragment color output" );
		return false;
		}

	NodeState *State = calloc ( Graph->NodeCount, sizeof ( NodeState ) );
	if ( State == NULL )
		return false;

	unsigned Index;
	for ( Index = 0; Index < Graph->NodeCount; ++Index )
		ComputeStage ( Graph, State, Index );

	Promote ( Graph, State, Graph->VertexPositionNode, scStage_Vertex );
	Promote ( Graph, State, Graph->FragmentColorNode, scStage_Fragment );

	for ( Index = 0; Index < Graph->NodeCount; ++Index )
		{
		State[Index].Visiting = false;
		if ( ( State[Index].Interpolate ) && ( Graph->Nodes[Index].VaryingName[0] == 0 ) )
			snprintf ( Graph->Nodes[Index].VaryingName, sizeof ( Graph->Nodes[Index].VaryingName ), "v_n%u", Index );
		}

	const unsigned GLSLVersion = EncodeGLSLVersion ( crGL4GetGLSLVersion () );
	StringBuilder VertexCode;
	StringBuilder FragmentCode;
	StringBuilder_Initialize ( &VertexCode );
	StringBuilder_Initialize ( &FragmentCode );

	StringBuilder_Appendf ( &VertexCode, "#version %u\n", GLSLVersion );
	StringBuilder_Appendf ( &FragmentCode, "#version %u\n", GLSLVersion );
	StringBuilder_Append ( &FragmentCode, "out vec4 FragOutput;\n" );

	for ( Index = 0; Index < Graph->NodeCount; ++Index )
		{
		const scNode *Node = &Graph->Nodes[Index];
		if ( Node->Type != scNodeType_Uniform )
			continue;
		char Declaration[128];
		if ( Node->ArrayCount > 0 )
			snprintf ( Declaration, sizeof ( Declaration ), "uniform %s %s[%u];\n", Stringify_scValueType ( Node->OutputTypes[0] ), Node->ResourceName, Node->ArrayCount );
		else
			snprintf ( Declaration, sizeof ( Declaration ), "uniform %s %s;\n", Stringify_scValueType ( Node->OutputTypes[0] ), Node->ResourceName );
		if ( State[Index].UsedVS )
			StringBuilder_Append ( &VertexCode, Declaration );
		if ( State[Index].UsedFS )
			StringBuilder_Append ( &FragmentCode, Declaration );
		}

	for ( Index = 0; Index < Graph->NodeCount; ++Index )
		{
		const scNode *Node = &Graph->Nodes[Index];
		if ( ( Node->Type == scNodeType_Attribute ) && ( State[Index].EmitVS || State[Index].Interpolate || State[Index].UsedVS ) )
			StringBuilder_Appendf ( &VertexCode, "in %s %s;\n", Stringify_scValueType ( Node->OutputTypes[0] ), Node->ResourceName );
		}

	for ( Index = 0; Index < Graph->NodeCount; ++Index )
		{
		if ( State[Index].Interpolate == false )
			continue;
		const scNode *Node = &Graph->Nodes[Index];
		const char *TypeName = Stringify_scValueType ( Node->OutputTypes[0] );
		StringBuilder_Appendf ( &VertexCode, "out %s %s;\n", TypeName, Node->VaryingName );
		StringBuilder_Appendf ( &FragmentCode, "in %s %s;\n", TypeName, Node->VaryingName );
		}

	if ( GraphHasType ( Graph, State, scNodeType_BlinnPhong, true ) )
		{
		StringBuilder_Append ( &FragmentCode, "\n" );
		StringBuilder_Append ( &FragmentCode, LightUniformsCode );
		StringBuilder_Append ( &FragmentCode, "\n" );
		StringBuilder_Append ( &FragmentCode, BlinnPhongLightingCode );
		}

	StringBuilder_Append ( &VertexCode, "\nvoid main()\n{\n" );
	StringBuilder_Append ( &FragmentCode, "\nvoid main()\n{\n" );

	bool Success = true;
	for ( Index = 0; Index < Graph->NodeCount && Success; ++Index )
		{
		if ( State[Index].EmitVS )
			Success = EmitNode ( Graph, State, &VertexCode, Index, scStage_Vertex );
		}

	for ( Index = 0; Index < Graph->NodeCount && Success; ++Index )
		{
		if ( State[Index].Interpolate == false )
			continue;
		char Expression[96];
		const scPin Pin = scNodePin ( Index );
		StringBuilder_Appendf ( &VertexCode, "%s = %s;\n", Graph->Nodes[Index].VaryingName,
		                        PinExpression ( Graph, State, Pin, scStage_Vertex, Expression, sizeof ( Expression ) ) );
		}

	if ( Success )
		{
		char Clip[96];
		StringBuilder_Appendf ( &VertexCode, "gl_Position = %s;\n",
		                        PinExpression ( Graph, State, Graph->Nodes[Graph->VertexPositionNode].Inputs[0], scStage_Vertex, Clip, sizeof ( Clip ) ) );
		}
	StringBuilder_Append ( &VertexCode, "}\n" );

	for ( Index = 0; Index < Graph->NodeCount && Success; ++Index )
		{
		if ( State[Index].EmitFS )
			Success = EmitNode ( Graph, State, &FragmentCode, Index, scStage_Fragment );
		}
	if ( Success )
		{
		char Color[96];
		StringBuilder_Appendf ( &FragmentCode, "FragOutput = %s;\n",
		                        PinExpression ( Graph, State, Graph->Nodes[Graph->FragmentColorNode].Inputs[0], scStage_Fragment, Color, sizeof ( Color ) ) );
		}
	StringBuilder_Append ( &FragmentCode, "}\n" );

	if ( Success )
		{
		OutputCode->VertexShader = StringBuilder_Take ( &VertexCode );
		OutputCode->FragmentShader = StringBuilder_Take ( &FragmentCode );
		}

	StringBuilder_Destroy ( &VertexCode );
	StringBuilder_Destroy ( &FragmentCode );
	free ( State );
	return Success;
	}

	#endif