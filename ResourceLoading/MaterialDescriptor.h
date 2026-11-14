#pragma once
#include <Platform/Platform.h>
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

struct MaterialDescriptor
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
	};

void LogMaterialDescriptor ( const struct MaterialDescriptor *Descriptor, const int Indentation );
void DestroyMaterialDescriptor ( struct MaterialDescriptor *Material );
void CopyMaterialDescriptor ( struct MaterialDescriptor *Output, const struct MaterialDescriptor *Input );
const char *Stringify_MatTexType ( const MatTexType Value );

END_C_DECLARATIONS
