#pragma once
#include <Platform/Platform.h>
#include <Math/mathlib.h>

BEGIN_C_DECLARATIONS
typedef struct
	{
	vec3 Position;
	quat Orientation;
	struct
		{
		mat4 TransformMatrix;
		vec3 XAxis, YAxis, ZAxis;
		bool Dirty;
		} CachedValues;
	} BaseTransformStruct;

void UpdateCachedValues ( BaseTransformStruct *Transform );
void SetOrientationFromAxisAngle ( BaseTransformStruct *Transform, const vec3 NewAxis, const float NewAngle );
void SetOrientationFromEulerAngles ( BaseTransformStruct *Transform, const vec3 Angles );
void RotateAroundAxisAngle ( BaseTransformStruct *Transform, const vec3 NewAxis, const float NewAngle );
void Rotate ( BaseTransformStruct *Transform, const quat Rotation );
void GetTransformationMatrix ( BaseTransformStruct *Transform, mat4 *Output );
void GetDirectionVectorsFromBaseTransformStruct ( BaseTransformStruct *Transform, vec3 *XAxis, vec3 *YAxis, vec3 *ZAxis );
END_C_DECLARATIONS
