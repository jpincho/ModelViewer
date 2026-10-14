#include "ModelData.h"
#include <Platform/Platform.h>

void DestroyModelData ( ModelData *Data )
	{
	SAFE_DEL_C ( Data->Meshes );
	Data->MeshCount = 0;
	if ( Data->NodeTree != NULL )
		{
		for ( unsigned Index = 0; Index < Data->NodeCount; ++Index )
			DestroyMeshTreeNode ( &Data->NodeTree[Index] );
		}
	SAFE_DEL_C ( Data->NodeTree );
	Data->NodeCount = 0;
	}
