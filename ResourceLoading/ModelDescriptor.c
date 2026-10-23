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

void ComputeMeshTreeNodeWorldMatrices ( const MeshTreeNode *NodeTree, const unsigned NodeCount, mat4 *WorldMatrices )
	{
	for ( unsigned Index = 0; Index < NodeCount; ++Index )
		{
		const int Parent = NodeTree[Index].ParentNode;
		if ( Parent < 0 )
			math_mat4_copy ( &WorldMatrices[Index], NodeTree[Index].DefaultTransformationMatrix );
		else if ( ( unsigned ) Parent < Index )
			math_mat4_multiply ( &WorldMatrices[Index], WorldMatrices[Parent], NodeTree[Index].DefaultTransformationMatrix );
		else
			{
			mat4 Accumulated;
			math_mat4_copy ( &Accumulated, NodeTree[Index].DefaultTransformationMatrix );
			int Walk = Parent;
			while ( Walk >= 0 )
				{
				mat4 Combined;
				math_mat4_multiply ( &Combined, NodeTree[Walk].DefaultTransformationMatrix, Accumulated );
				math_mat4_copy ( &Accumulated, Combined );
				Walk = NodeTree[Walk].ParentNode;
				}
			math_mat4_copy ( &WorldMatrices[Index], Accumulated );
			}
		}
	}

void CalculateModelDescriptorAABB ( ModelDescriptor *Descriptor )
	{
	AABB_InitEmpty ( &Descriptor->AABBox );

	// Calculate the bounding boxes for all the nodes in this model, using the tree and the matrices
	if ( Descriptor->NodeCount != 0 )
		{
		mat4 *NodeWorldMatrices = alloca ( Descriptor->NodeCount * sizeof ( mat4 ) );
		ComputeMeshTreeNodeWorldMatrices ( Descriptor->Nodes, Descriptor->NodeCount, NodeWorldMatrices );
		for ( unsigned NodeIndex = 0; NodeIndex < Descriptor->NodeCount; ++NodeIndex )
			{
			for ( unsigned MeshIndex = 0; MeshIndex < Descriptor->Nodes[NodeIndex].MeshIndexCount; ++MeshIndex )
				{
				const unsigned MeshArrayIndex = Descriptor->Nodes[NodeIndex].MeshIndices[MeshIndex];
				if ( MeshArrayIndex >= Descriptor->MeshCount )
					continue;
				AABB TransformedBox;
				AABB_Transform ( &TransformedBox, &Descriptor->Meshes[MeshArrayIndex].BoundingBox, &NodeWorldMatrices[NodeIndex] );
				AABB_Join ( &Descriptor->AABBox, &Descriptor->AABBox, &TransformedBox );
				}
			}
		}
	}