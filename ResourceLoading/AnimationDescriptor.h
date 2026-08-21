#pragma once
#include <Platform/defines.h>
#include <Math/mathlib.h>

typedef struct
	{
	double time;
	union
		{
		vec3 Vec3Value;
		quat QuatValue;
		};
	} NodeTrackKey;

typedef struct
	{
	char *NodeName;
	NodeTrackKey *Positions, *Scales, *Rotations;
	unsigned PositionCount, ScaleCount, RotationCount;
	} NodeTrack;

typedef struct
	{
	NodeTrack *NodeTracks;
	unsigned NodeTrackCount;
	double Duration;
	char *AnimationName;
	} AnimationDescriptor;

BEGIN_C_DECLARATIONS
void DestroyAnimationDescriptor ( AnimationDescriptor *Descriptor );
END_C_DECLARATIONS
