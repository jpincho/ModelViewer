#include "AnimationDescriptor.h"
#include <stdlib.h>

void DestroyAnimationDescriptor ( AnimationDescriptor *Descriptor )
	{
	for ( unsigned index = 0; index < Descriptor->NodeTrackCount; ++index )
		{
		SAFE_DEL_C ( Descriptor->NodeTracks[index].NodeName );
		SAFE_DEL_C ( Descriptor->NodeTracks[index].Rotations );
		SAFE_DEL_C ( Descriptor->NodeTracks[index].Positions );
		SAFE_DEL_C ( Descriptor->NodeTracks[index].Scales );
		}
	SAFE_DEL_C ( Descriptor->NodeTracks );
	SAFE_DEL_C ( Descriptor->AnimationName );
	}
