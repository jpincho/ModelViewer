#include "BaseTransform.h"

void UpdateCachedValues ( BaseTransformStruct *Transform )
	{
	math_vec3_rotate_by_quat ( &Transform->CachedValues.ZAxis, MAKE_VEC3 ( vec3, 0.0f, 0.0f, 1.0f ), Transform->Orientation );
	math_vec3_rotate_by_quat ( &Transform->CachedValues.YAxis, MAKE_VEC3 ( vec3, 0.0f, 1.0f, 0.0f ), Transform->Orientation );
	math_vec3_rotate_by_quat ( &Transform->CachedValues.XAxis, MAKE_VEC3 ( vec3, 1.0f, 0.0f, 0.0f ), Transform->Orientation );

	mat4 Orientation;
	mat4 Position;
	math_mat4_set_translation_matrix ( &Position, Transform->Position );
	math_quat_to_mat4 ( &Orientation, Transform->Orientation );
	math_mat4_multiply ( &Transform->CachedValues.TransformMatrix, Position, Orientation );
	Transform->CachedValues.TransformMatrix.flat_raw[15] = 1.0f;
	Transform->CachedValues.Dirty = false;
	}

void SetOrientationFromAxisAngle ( BaseTransformStruct *Transform, const vec3 NewAxis, const float NewAngle )
	{
	math_quat_from_axis_angle ( &Transform->Orientation, NewAxis, NewAngle );
	math_quat_normalize ( &Transform->Orientation );
	Transform->CachedValues.Dirty = true;
	}

void SetOrientationFromEulerAngles ( BaseTransformStruct *Transform, const vec3 Angles )
	{
	math_quat_from_euler ( &Transform->Orientation, Angles );
	math_quat_normalize ( &Transform->Orientation );
	Transform->CachedValues.Dirty = true;
	}

void RotateAroundAxisAngle ( BaseTransformStruct *Transform, const vec3 NewAxis, const float NewAngle )
	{
	quat Rotation;
	math_quat_from_axis_angle ( &Rotation, NewAxis, NewAngle );
	math_quat_multiply_by_quat ( &Transform->Orientation, Transform->Orientation, Rotation );
	math_quat_normalize ( &Transform->Orientation );
	Transform->CachedValues.Dirty = true;
	}

void Rotate ( BaseTransformStruct *Transform, const quat Rotation )
	{
	math_quat_multiply_by_quat ( &Transform->Orientation, Transform->Orientation, Rotation );
	math_quat_normalize ( &Transform->Orientation );
	Transform->CachedValues.Dirty = true;
	}

void GetTransformationMatrix ( BaseTransformStruct *Transform, mat4 *Output )
	{
	if ( Transform->CachedValues.Dirty == true )
		UpdateCachedValues ( Transform );
	math_mat4_copy ( Output, Transform->CachedValues.TransformMatrix );
	}

void GetDirectionVectorsFromBaseTransformStruct ( BaseTransformStruct *Transform, vec3 *XAxis, vec3 *YAxis, vec3 *ZAxis )
	{
	if ( Transform->CachedValues.Dirty == true )
		UpdateCachedValues ( Transform );
	/*
		Front->x = 2 * (Transform->Orientation.x * Transform->Orientation.z + Transform->Orientation.w * Transform->Orientation.y);
		Front->y = 2 * (Transform->Orientation.y * Transform->Orientation.z - Transform->Orientation.w * Transform->Orientation.x);
		Front->z = 1 - 2 * (Transform->Orientation.x * Transform->Orientation.x + Transform->Orientation.y * Transform->Orientation.y);
		math_vec3_normalize(Front);

		Up->x = 2 * (Transform->Orientation.x * Transform->Orientation.y - Transform->Orientation.w * Transform->Orientation.z);
		Up->y = 1 - 2 * (Transform->Orientation.x * Transform->Orientation.x + Transform->Orientation.z * Transform->Orientation.z);
		Up->z = 2 * (Transform->Orientation.y * Transform->Orientation.z + Transform->Orientation.w * Transform->Orientation.x);
		math_vec3_normalize(Up);

		Right->x = 1 - 2 * (Transform->Orientation.y * Transform->Orientation.y + Transform->Orientation.z * Transform->Orientation.z);
		Right->y = 2 * (Transform->Orientation.x * Transform->Orientation.y + Transform->Orientation.w * Transform->Orientation.z);
		Right->z = 2 * (Transform->Orientation.x * Transform->Orientation.z - Transform->Orientation.w * Transform->Orientation.y);
		math_vec3_normalize(Right);
		*/

	math_vec3_copy ( XAxis, Transform->CachedValues.XAxis );
	math_vec3_copy ( YAxis, Transform->CachedValues.YAxis );
	math_vec3_copy ( ZAxis, Transform->CachedValues.ZAxis );
	}

