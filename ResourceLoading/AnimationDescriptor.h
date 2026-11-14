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

struct AnimationDescriptor
	{
	NodeTrack *NodeTracks;
	unsigned NodeTrackCount;
	double Duration;
	char *AnimationName;
	};

BEGIN_C_DECLARATIONS
void LogAnimationDescriptor ( const struct AnimationDescriptor *Descriptor, const int Indentation );
void CopyAnimationDescriptor ( struct AnimationDescriptor *Output, const struct AnimationDescriptor *Input );
void DestroyAnimationDescriptor ( struct AnimationDescriptor *Descriptor );
END_C_DECLARATIONS
