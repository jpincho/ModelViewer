#pragma once
#include <stdbool.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include "../ResourceLoading/MaterialDescriptor.h"

BEGIN_C_DECLARATIONS

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
	scAttribute_Shininess,
	scAttribute_Emissiveness,
	scAttribute_Count
	} scShaderAttributeType;

bool scInitializeShaders ( void );
bool scShutdownShaders ( void );
crShaderHandle scAssembleShader ( const scShaderDefinition NewDefinition );
void scCreateShaderCode ( const scShaderDefinition NewRequirements, crShaderCode *OutputCode );

const char *scGetStandardTextureUniformName ( const MatTexType Type );
const char *scGetStandardMaterialUniformName ( const MatTexType Type );
const char *scGetStandardAttributeName ( const scShaderAttributeType Type );
const char *Stringify_ShaderAttributeType ( const scShaderAttributeType Type );

END_C_DECLARATIONS
