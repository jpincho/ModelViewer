#pragma once
#include <Math/mathlib.h>

typedef struct
	{
	vec3 Center;
	float Radius;
	} BoundingSphere;

typedef struct
	{
	vec3 Min, Max;
	} AABB;

void BoundingSphere_InitEmpty ( BoundingSphere *Sphere );

void AABB_InitEmpty ( AABB *Box );
void AABB_Join ( AABB *Output, const AABB *First, const AABB *Second );
void AABB_IncludePoint ( AABB *Box, const vec3 Point );
void AABB_Transform ( AABB *Output, const AABB *Input, const mat4 *Transform );
bool AABB_DoBoxesIntersect ( const AABB Box1, const AABB Box2 );
bool AABB_FitsInside ( const AABB Outer, const AABB Inner );
void AABB_CalculateCorners ( const AABB Box, vec3 Corners[8] );