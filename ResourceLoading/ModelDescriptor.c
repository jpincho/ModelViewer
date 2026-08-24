#include "ModelDescriptor.h"
#include <string.h>
#include <Platform/Logger.h>

void LogModelDescriptor ( const ModelDescriptor Descriptor, const int Indentation )
	{
	LOG_DEBUG ( "%*sModel '%s' - Mesh count %u", Indentation, "", Descriptor.Name, Descriptor.MeshCount );
	for ( unsigned Index = 0; Index < Descriptor.MeshCount; ++Index )
		{
		LogMeshDescriptor ( Descriptor.Meshes[Index], Indentation + 1 );
		}
	}

void CopyModelDescriptor ( ModelDescriptor *Output, const ModelDescriptor *Input )
	{
	Output->Name = Input->Name ? strdup ( Input->Name ) : NULL;
	Output->MeshCount = Input->MeshCount;
	if ( Input->MeshCount )
		{
		Output->Meshes = calloc ( Output->MeshCount, sizeof ( MeshDescriptor ) );
		ASSERT_FAIL ( Output->Meshes != NULL );
		for ( unsigned MeshIndex = 0; MeshIndex < Input->MeshCount; ++MeshIndex )
			{
			CopyMeshDescriptor ( Output->Meshes + MeshIndex, Input->Meshes + MeshIndex );
			}
		}
	Output->NodeCount = Input->NodeCount;
	if ( Input->NodeCount )
		{
		Output->Nodes = calloc ( Input->NodeCount, sizeof ( MeshTreeNode ) );
		ASSERT_FAIL ( Output->Nodes != NULL );
		for ( unsigned NodeIndex = 0; NodeIndex < Input->NodeCount; ++NodeIndex )
			{
			CopyMeshTreeNode ( Output->Nodes + NodeIndex, Input->Nodes + NodeIndex );
			}
		}
	memcpy ( &Output->AABBox, &Input->AABBox, sizeof ( AABB ) );
	}

void DestroyModelDescriptor ( ModelDescriptor *Descriptor )
	{
	SAFE_DEL_C ( Descriptor->Name );
	for ( unsigned MeshIndex = 0; MeshIndex < Descriptor->MeshCount; ++MeshIndex )
		{
		DestroyMeshDescriptor ( Descriptor->Meshes + MeshIndex );
		}
	SAFE_DEL_C ( Descriptor->Meshes );
	Descriptor->MeshCount = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < Descriptor->NodeCount; ++NodeIndex )
		{
		DestroyMeshTreeNode ( Descriptor->Nodes + NodeIndex );
		}
	SAFE_DEL_C ( Descriptor->Nodes );
	Descriptor->NodeCount = 0;
	}
