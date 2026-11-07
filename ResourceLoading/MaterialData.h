#pragma once
#include "MaterialDescriptor.h"

struct MaterialData
	{
	vec3 Diffuse, Specular, Emissive;
	float Shininess;
	crTextureBindSettings Textures[MatTexType_Count];
	} ;

