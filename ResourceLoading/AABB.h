#pragma once
#include <Math/mathlib.h>

struct BoundingSphere
	{
	vec3 Center;
	float Radius;
	};

struct AABB
	{
	vec3 Min, Max;
	};

void BoundingSphere_InitEmpty ( struct BoundingSphere *Sphere );

void AABB_InitEmpty ( struct AABB *Box );
void AABB_Join ( struct AABB *Output, const struct AABB *First, const struct AABB *Second );
void AABB_IncludePoint ( struct AABB *Box, const vec3 Point );
void AABB_Transform ( struct AABB *Output, const struct AABB *Input, const mat4 *Transform );
bool AABB_DoBoxesIntersect ( const struct AABB Box1, const struct AABB Box2 );
bool AABB_FitsInside ( const struct AABB Outer, const struct AABB Inner );
void AABB_CalculateCorners ( const struct AABB Box, vec3 Corners[8] );