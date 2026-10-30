#pragma once
#include <Platform/Platform.h>
#include <Math/mathlib.h>

BEGIN_C_DECLARATIONS
struct BaseTransform
	{
	vec3 Position;
	quat Orientation;
	struct
		{
		mat4 TransformMatrix;
		vec3 XAxis, YAxis, ZAxis;
		bool Dirty;
		} CachedValues;
	};

void UpdateCachedValues ( struct BaseTransform *Transform );
void SetOrientationFromAxisAngle ( struct BaseTransform *Transform, const vec3 NewAxis, const float NewAngle );
void SetOrientationFromEulerAngles ( struct BaseTransform *Transform, const vec3 Angles );
void RotateAroundAxisAngle ( struct BaseTransform *Transform, const vec3 NewAxis, const float NewAngle );
void Rotate ( struct BaseTransform *Transform, const quat Rotation );
void GetTransformationMatrix ( struct BaseTransform *Transform, mat4 *Output );
void GetDirectionVectorsFromBaseTransformStruct ( struct BaseTransform *Transform, vec3 *XAxis, vec3 *YAxis, vec3 *ZAxis );
END_C_DECLARATIONS
