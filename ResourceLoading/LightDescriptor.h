#pragma once
#include <Math/mathlib.h>

typedef enum
	{
	LightType_Point,
	LightType_Spot,
	LightType_Directional,
	LightType_Ambient
	} LightType;

typedef struct
	{
	LightType Type;
	char *Name;
	vec3 Position;
	vec3 Direction;
	vec3 Ambient, Diffuse, Specular;
	float Intensity;
	float ConstantAttenuation;
	float LinearAttenuation;
	float ExponentialAttenuation;
	float InnerCutoffAngle, OuterCutoffAngle;
	} LightDescriptor;

typedef struct
	{
	vec3 Color;
	float Intensity;
	} AmbientLightDescriptor;
