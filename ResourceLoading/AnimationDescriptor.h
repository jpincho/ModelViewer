#pragma once
#include <Platform/Platform.h>
#include <Math/mathlib.h>

typedef struct
	{
	double Time;
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
void LogAnimationDescriptor ( const AnimationDescriptor *Descriptor, const int Indentation );
void CopyAnimationDescriptor ( AnimationDescriptor *Output, const AnimationDescriptor *Input );
void DestroyAnimationDescriptor ( AnimationDescriptor *Descriptor );
END_C_DECLARATIONS
