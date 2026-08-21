#pragma once
#include "MaterialDescriptor.h"

typedef struct
	{
	vec3 Diffuse, Specular, Emissive;
	float Shininess;
	crTextureBindSettings Textures[MatTexType_Count];
	} MaterialData;

