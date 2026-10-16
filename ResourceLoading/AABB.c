#include "AABB.h"
#include <float.h>

void BoundingSphere_InitEmpty ( BoundingSphere *Sphere )
	{
	Sphere->Center = MATH_VEC3_ZERO;
	Sphere->Radius = 0.0f;
	}

void AABB_InitEmpty ( AABB *Box )
	{
	Box->Min.x = Box->Min.y = Box->Min.z = FLT_MAX;
	Box->Max.x = Box->Max.y = Box->Max.z = -FLT_MAX;
	}

void AABB_Join ( AABB *Output, const AABB *First, const AABB *Second )
	{
	Output->Min.x = fminf ( First->Min.x, Second->Min.x );
	Output->Min.y = fminf ( First->Min.y, Second->Min.y );
	Output->Min.z = fminf ( First->Min.z, Second->Min.z );
	Output->Max.x = fmaxf ( First->Max.x, Second->Max.x );
	Output->Max.y = fmaxf ( First->Max.y, Second->Max.y );
	Output->Max.z = fmaxf ( First->Max.z, Second->Max.z );
	}

void AABB_IncludePoint ( AABB *Box, const vec3 Point )
	{
	Box->Min.x = fminf ( Box->Min.x, Point.x );
	Box->Min.y = fminf ( Box->Min.y, Point.y );
	Box->Min.z = fminf ( Box->Min.z, Point.z );
	Box->Max.x = fmaxf ( Box->Max.x, Point.x );
	Box->Max.y = fmaxf ( Box->Max.y, Point.y );
	Box->Max.z = fmaxf ( Box->Max.z, Point.z );
	}

void AABB_Transform ( AABB *Output, const AABB *Input, const mat4 *Transform )
	{
	AABB_InitEmpty ( Output );

	// Transform all 8 corners
	vec4 Corners[8] =
		{
			{Input->Min.x, Input->Min.y, Input->Min.z, 1.0f},
			{Input->Min.x, Input->Min.y, Input->Max.z, 1.0f},
			{Input->Min.x, Input->Max.y, Input->Min.z, 1.0f},
			{Input->Min.x, Input->Max.y, Input->Max.z, 1.0f},
			{Input->Max.x, Input->Min.y, Input->Min.z, 1.0f},
			{Input->Max.x, Input->Min.y, Input->Max.z, 1.0f},
			{Input->Max.x, Input->Max.y, Input->Min.z, 1.0f},
			{Input->Max.x, Input->Max.y, Input->Max.z, 1.0f}
		};

	for ( int CornerIndex = 0; CornerIndex < 8; ++CornerIndex )
		{
		vec4 Transformed4;
		math_vec4_multiply_by_mat4 ( &Transformed4, *Transform, Corners[CornerIndex] );
		vec3 Transformed3 = {Transformed4.x, Transformed4.y, Transformed4.z};
		AABB_IncludePoint ( Output, Transformed3 );
		}
	}

bool AABB_DoBoxesIntersect ( const AABB Box1, const AABB Box2 )
	{
	if ( ( ( Box1.Min.x > Box2.Max.x ) || ( Box1.Max.x < Box2.Min.x ) ) ||
	        ( ( Box1.Min.y > Box2.Max.y ) || ( Box1.Max.y < Box2.Min.y ) ) ||
	        ( ( Box1.Min.z > Box2.Max.z ) || ( Box1.Max.z < Box2.Min.z ) ) )
		return false;
	return true;
	}

bool AABB_FitsInside ( const AABB Outer, const AABB Inner )
	{
	return ( ( Inner.Min.x >= Outer.Min.x ) && ( Inner.Max.x <= Outer.Max.x ) &&
	         ( Inner.Min.y >= Outer.Min.y ) && ( Inner.Max.y <= Outer.Max.y ) &&
	         ( Inner.Min.z >= Outer.Min.z ) && ( Inner.Max.z <= Outer.Max.z ) );
	}

void AABB_CalculateCorners ( const AABB Box, vec3 Corners[8] )
	{
	Corners[0] = MAKE_VEC3 ( Box.Min.x, Box.Min.y, Box.Min.z );
	Corners[1] = MAKE_VEC3 ( Box.Min.x, Box.Min.y, Box.Max.z );
	Corners[2] = MAKE_VEC3 ( Box.Min.x, Box.Max.y, Box.Min.z );
	Corners[3] = MAKE_VEC3 ( Box.Min.x, Box.Max.y, Box.Max.z );
	Corners[4] = MAKE_VEC3 ( Box.Max.x, Box.Min.y, Box.Min.z );
	Corners[5] = MAKE_VEC3 ( Box.Max.x, Box.Min.y, Box.Max.z );
	Corners[6] = MAKE_VEC3 ( Box.Max.x, Box.Max.y, Box.Min.z );
	Corners[7] = MAKE_VEC3 ( Box.Max.x, Box.Max.y, Box.Max.z );
	}