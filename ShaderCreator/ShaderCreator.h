#pragma once
#include <stdbool.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include "../ResourceLoading/MaterialDescriptor.h"

BEGIN_C_DECLARATIONS

typedef unsigned scNodeId;
#define scNodeId_Invalid ( ( scNodeId ) 0xFFFFFFFFu )

typedef enum
	{
	scValue_Float = 0,
	scValue_Vec2,
	scValue_Vec3,
	scValue_Vec4,
	scValue_UVec4,
	scValue_Mat3,
	scValue_Mat4,
	scValue_Sampler2D,
	scValue_Void
	} scValueType;

typedef enum
	{
	scNodeType_Attribute = 0,
	scNodeType_Uniform,
	scNodeType_Constant,
	scNodeType_TextureSample,
	scNodeType_Multiply,
	scNodeType_Add,
	scNodeType_Normalize,
	scNodeType_ComposeVec4,
	scNodeType_Swizzle,
	scNodeType_Mat3Cast,
	scNodeType_Skinning,
	scNodeType_TBN,
	scNodeType_NormalMap,
	scNodeType_BlinnPhong,
	scNodeType_VertexPosition,
	scNodeType_FragmentColor
	} scNodeType;

enum
	{
	scSkinningOut_BoneMatrix = 0,
	scSkinningOut_Position = 1
	};

typedef struct
	{
	scNodeId Node;
	unsigned Output;
	} scPin;

typedef struct scShaderGraph scShaderGraph;

typedef enum
	{
	scSource_Off = 0,
	scSource_Uniform,
	scSource_Attribute,
	scSource_Texture
	} scMaterialSource;

typedef enum
	{
	scLightModel_Off = 0,
	scLightModel_BlinnPhong
	} scLightModel;

typedef struct
	{
	scMaterialSource DiffuseSource, SpecularSource, ShininessSource, EmissivenessSource, NormalSource;
	scLightModel LightModel;
	unsigned BoneCount;
	bool Skinning;
	} scShaderDefinition;

typedef enum
	{
	scAttribute_Position = 0,
	scAttribute_TextureCoordinate,
	scAttribute_Normal,
	scAttribute_Color,
	scAttribute_Tangent,
	scAttribute_Bitangent,
	scAttribute_BoneIndex,
	scAttribute_BoneWeight,
	scAttribute_Count
	} scShaderAttributeType;

static inline scPin scMakePin ( const scNodeId Node, const unsigned Output )
	{
	scPin Pin;
	Pin.Node = Node;
	Pin.Output = Output;
	return Pin;
	}

static inline scPin scNodePin ( const scNodeId Node )
	{
	return scMakePin ( Node, 0 );
	}

scShaderGraph *scGraph_Create ( void );
void scGraph_Destroy ( scShaderGraph *Graph );

scNodeId scGraph_AddAttribute ( scShaderGraph *Graph, const char *Name, const scValueType Type );
scNodeId scGraph_AddUniform ( scShaderGraph *Graph, const char *Name, const scValueType Type );
scNodeId scGraph_AddUniformArray ( scShaderGraph *Graph, const char *Name, const scValueType Type, const unsigned Count );
scNodeId scGraph_AddConstantFloat ( scShaderGraph *Graph, const float Value );
scNodeId scGraph_AddConstantVec3 ( scShaderGraph *Graph, const float X, const float Y, const float Z );
scNodeId scGraph_AddConstantVec4 ( scShaderGraph *Graph, const float X, const float Y, const float Z, const float W );
scNodeId scGraph_AddIdentityMat4 ( scShaderGraph *Graph );

scNodeId scGraph_AddTextureSample ( scShaderGraph *Graph, const scPin Sampler, const scPin UV );
scNodeId scGraph_AddMultiply ( scShaderGraph *Graph, const scPin A, const scPin B );
scNodeId scGraph_AddAdd ( scShaderGraph *Graph, const scPin A, const scPin B );
scNodeId scGraph_AddNormalize ( scShaderGraph *Graph, const scPin Value );
scNodeId scGraph_AddComposeVec4 ( scShaderGraph *Graph, const scPin XYZ, const scPin W );
scNodeId scGraph_AddSwizzle ( scShaderGraph *Graph, const scPin Value, const char *Swizzle );
scNodeId scGraph_AddMat3Cast ( scShaderGraph *Graph, const scPin Matrix4 );
scNodeId scGraph_AddSkinning ( scShaderGraph *Graph, const scPin Position, const scPin BoneIndex, const scPin BoneWeight, const scPin Bones );
scNodeId scGraph_AddTBN ( scShaderGraph *Graph, const scPin Tangent, const scPin Bitangent, const scPin Normal, const scPin ModelMatrix, const scPin BoneMatrix );
scNodeId scGraph_AddNormalMap ( scShaderGraph *Graph, const scPin Sampler, const scPin UV, const scPin TBN );
scNodeId scGraph_AddBlinnPhong ( scShaderGraph *Graph, const scPin Diffuse, const scPin Specular, const scPin Shininess, const scPin Normal, const scPin WorldPosition, const scPin ViewPosition );
scNodeId scGraph_AddVertexPosition ( scShaderGraph *Graph, const scPin ClipPosition );
scNodeId scGraph_AddFragmentColor ( scShaderGraph *Graph, const scPin Color );

bool scGraph_SetVaryingName ( scShaderGraph *Graph, const scNodeId Node, const char *Name );
bool scGraph_GenerateCode ( scShaderGraph *Graph, crShaderCode *OutputCode );
scShaderGraph *scGraph_BuildFromDefinition ( const scShaderDefinition Definition );

bool scInitializeShaders ( void );
bool scShutdownShaders ( void );
crShaderHandle scAssembleShader ( const scShaderDefinition NewDefinition );
void scCreateShaderCode ( const scShaderDefinition NewRequirements, crShaderCode *OutputCode );

const char *scGetStandardTextureUniformName ( const MatTexType Type );
const char *scGetStandardMaterialUniformName ( const MatTexType Type );
const char *scGetStandardAttributeName ( const scShaderAttributeType Type );
const char *Stringify_ShaderAttributeType ( const scShaderAttributeType Type );
const char *Stringify_scValueType ( const scValueType Type );
const char *Stringify_scNodeType ( const scNodeType Type );

END_C_DECLARATIONS
