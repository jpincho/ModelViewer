#include "AnimationDescriptor.h"
#include "MeshDescriptor.h"
#include <Platform/Logger.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

static void CopyNodeTrack ( NodeTrack *Output, const NodeTrack *Input )
	{
	Output->NodeName = Input->NodeName ? strdup ( Input->NodeName ) : NULL;
	Output->PositionCount = Input->PositionCount;
	Output->RotationCount = Input->RotationCount;
	Output->ScaleCount = Input->ScaleCount;
	if ( ( Input->PositionCount != 0 ) && ( Input->Positions != NULL ) )
		{
		Output->Positions = malloc ( sizeof ( NodeTrackKey ) * Input->PositionCount );
		ASSERT_FAIL ( Output->Positions != NULL );
		memcpy ( Output->Positions, Input->Positions, sizeof ( NodeTrackKey ) * Input->PositionCount );
		}
	if ( ( Input->RotationCount != 0 ) && ( Input->Rotations != NULL ) )
		{
		Output->Rotations = malloc ( sizeof ( NodeTrackKey ) * Input->RotationCount );
		ASSERT_FAIL ( Output->Rotations != NULL );
		memcpy ( Output->Rotations, Input->Rotations, sizeof ( NodeTrackKey ) * Input->RotationCount );
		}
	if ( ( Input->ScaleCount != 0 ) && ( Input->Scales != NULL ) )
		{
		Output->Scales = malloc ( sizeof ( NodeTrackKey ) * Input->ScaleCount );
		ASSERT_FAIL ( Output->Scales != NULL );
		memcpy ( Output->Scales, Input->Scales, sizeof ( NodeTrackKey ) * Input->ScaleCount );
		}
	}

static void DestroyNodeTrack ( NodeTrack *Track )
	{
	SAFE_DEL_C ( Track->NodeName );
	SAFE_DEL_C ( Track->Positions );
	SAFE_DEL_C ( Track->Rotations );
	SAFE_DEL_C ( Track->Scales );
	}

void LogAnimationDescriptor ( const AnimationDescriptor *Descriptor, const int Indentation )
	{
	LOG_DEBUG ( "%*sAnimation '%s'", Indentation, "", Descriptor->AnimationName );
	LOG_DEBUG ( "%*sDuration %.02f", Indentation + 1, "", Descriptor->Duration );
	LOG_DEBUG ( "%*sTrack Count %u", Indentation + 1, "", Descriptor->NodeTrackCount );
	for ( unsigned TrackIndex = 0; TrackIndex < Descriptor->NodeTrackCount; ++TrackIndex )
		{
		LOG_DEBUG ( "%*s'%s'", Indentation + 2, "", Descriptor->NodeTracks[TrackIndex].NodeName );
		}
	}

void CopyAnimationDescriptor ( AnimationDescriptor *Output, const AnimationDescriptor *Input )
	{
	Output->AnimationName = Input->AnimationName ? strdup ( Input->AnimationName ) : NULL;
	Output->Duration = Input->Duration;
	Output->NodeTrackCount = Input->NodeTrackCount;
	Output->NodeTracks = NULL;
	if ( ( Input->NodeTrackCount == 0 ) || ( Input->NodeTracks == NULL ) )
		return;
	Output->NodeTracks = calloc ( Input->NodeTrackCount, sizeof ( NodeTrack ) );
	ASSERT_FAIL ( Output->NodeTracks != NULL );
	for ( unsigned Index = 0; Index < Input->NodeTrackCount; ++Index )
		CopyNodeTrack ( &Output->NodeTracks[Index], &Input->NodeTracks[Index] );
	}

void DestroyAnimationDescriptor ( AnimationDescriptor *Descriptor )
	{
	for ( unsigned TrackIndex = 0; TrackIndex < Descriptor->NodeTrackCount; ++TrackIndex )
		{
		DestroyNodeTrack ( &Descriptor->NodeTracks[TrackIndex] );
		}
	SAFE_DEL_C ( Descriptor->NodeTracks );
	SAFE_DEL_C ( Descriptor->AnimationName );
	Descriptor->NodeTrackCount = 0;
	Descriptor->Duration = 0.0;
	}
