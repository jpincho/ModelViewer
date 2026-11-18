#include "ModelData.h"
#include <Platform/Platform.h>

void DestroyModelData ( struct ModelData *Data )
	{
	SAFE_DEL_C ( Data->Meshes );
	Data->MeshCount = 0;
	if ( Data->NodeTree != NULL )
		{
		for ( unsigned Index = 0; Index < Data->NodeCount; ++Index )
			DestroyMeshTreeNode ( &Data->NodeTree[Index] );
		}
	SAFE_DEL_C ( Data->NodeTree );
	SAFE_DEL_C ( Data->MeshNodeTreeWorldMatrices );
	Data->NodeCount = 0;
	}
