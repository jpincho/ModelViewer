#pragma once
#include <Platform/defines.h>
#include <C_CrossRenderer/Texture.h>

BEGIN_C_DECLARATIONS

typedef enum
	{
	MatTexType_Diffuse,
	MatTexType_Specular,
	MatTexType_Ambient,
	MatTexType_Emissive,
	MatTexType_Height,
	MatTexType_Normal,
	MatTexType_Shininess,
	MatTexType_Opacity,
	MatTexType_Displacement,
	MatTexType_Lightmap,
	MatTexType_Reflection,
	MatTexType_Count
	} MatTexType;

typedef struct
	{
	char *MaterialName;
	vec3 Diffuse, Specular, Emissive;
	float Shininess;
	struct
		{
		char *TextureName;
		crTextureWrapSettings WrapSettings;
		crTextureFilterSettings FilterSettings;
		} Textures[MatTexType_Count];
	} MaterialDescriptor;

void LogMaterialDescriptor ( const MaterialDescriptor *Descriptor, const int Indentation );
void DestroyMaterialDescriptor ( MaterialDescriptor *Material );
void CopyMaterialDescriptor ( MaterialDescriptor *Output, const MaterialDescriptor *Input );
const char *Stringify_MatTexType ( const MatTexType Value );

END_C_DECLARATIONS
