#include "MeshDescriptor.h"
#include <string.h>
#include <Platform/Logger.h>

void LogMeshDescriptor ( const MeshDescriptor Descriptor, const int Indentation )
	{
	LOG_DEBUG ( "%*sMesh '%s'", Indentation, "", Descriptor.MeshName );
	LOG_DEBUG ( "%*sMaterial - '%s'", Indentation + 1, "", Descriptor.MaterialName );
	LOG_DEBUG ( "%*sVertices - %u", Indentation + 1, "", Descriptor.VertexCount );
	LOG_DEBUG ( "%*sIndices - %u", Indentation + 1, "", Descriptor.IndexCount );
	}

void CopyMeshBone ( MeshBone *Output, const MeshBone *Input )
	{
	Output->NodeIndex = Input->NodeIndex;
	Output->BoneName = strdup ( Input->BoneName );
	math_mat4_copy ( &Output->ModelToBoneMatrix, Input->ModelToBoneMatrix );
	}

void DestroyMeshBone ( MeshBone *Bone )
	{
	SAFE_DEL_C ( Bone->BoneName );
	}

void CopyMeshDescriptor ( MeshDescriptor *Output, const MeshDescriptor *Input )
	{
	if ( Input->VertexCount == 0 )
		return;
	if ( Input->Colors != NULL )
		{
		Output->Colors = malloc ( sizeof ( vec4 ) * Input->VertexCount );
		ASSERT_FAIL ( Output->Colors != NULL );
		memcpy ( Output->Colors, Input->Colors, sizeof ( vec4 ) * Input->VertexCount );
		}

	Output->Positions = malloc ( sizeof ( vec3 ) * Input->VertexCount );
	ASSERT_FAIL ( Output->Positions != NULL );
	memcpy ( Output->Positions, Input->Positions, sizeof ( vec3 ) * Input->VertexCount );

	if ( Input->Normals != NULL )
		{
		Output->Normals = malloc ( sizeof ( vec3 ) * Input->VertexCount );
		ASSERT_FAIL ( Output->Normals != NULL );
		memcpy ( Output->Normals, Input->Normals, sizeof ( vec3 ) * Input->VertexCount );
		}
	if ( Input->Tangents != NULL )
		{
		Output->Tangents = malloc ( sizeof ( vec3 ) * Input->VertexCount );
		ASSERT_FAIL ( Output->Tangents != NULL );
		memcpy ( Output->Tangents, Input->Tangents, sizeof ( vec3 ) * Input->VertexCount );
		}
	if ( Input->Bitangents != NULL )
		{
		Output->Bitangents = malloc ( sizeof ( vec3 ) * Input->VertexCount );
		ASSERT_FAIL ( Output->Bitangents != NULL );
		memcpy ( Output->Bitangents, Input->Bitangents, sizeof ( vec3 ) * Input->VertexCount );
		}
	if ( Input->TexCoords != NULL )
		{
		Output->TexCoords = malloc ( sizeof ( vec2 ) * Input->VertexCount );
		ASSERT_FAIL ( Output->TexCoords != NULL );
		memcpy ( Output->TexCoords, Input->TexCoords, sizeof ( vec2 ) * Input->VertexCount );
		}

	if ( Input->Indices != NULL )
		{
		Output->Indices = malloc ( sizeof ( uint32_t ) * Input->IndexCount );
		ASSERT_FAIL ( Output->Indices != NULL );
		memcpy ( Output->Indices, Input->Indices, sizeof ( uint32_t ) * Input->IndexCount );
		}

	if ( Input->BoneWeights != NULL )
		{
		Output->BoneWeights = malloc ( sizeof ( float ) * Input->VertexCount * 4 );
		ASSERT_FAIL ( Output->BoneWeights != NULL );
		memcpy ( Output->BoneWeights, Input->BoneWeights, sizeof ( float ) * Input->VertexCount * 4 );
		}

	if ( Input->Bones != NULL )
		{
		Output->Bones = malloc ( sizeof ( MeshBone ) * Input->BoneCount );
		ASSERT_FAIL ( Output->Bones != NULL );
		for ( unsigned BoneIndex = 0; BoneIndex < Input->BoneCount; ++BoneIndex )
			CopyMeshBone ( &Output->Bones[BoneIndex], &Input->Bones[BoneIndex] );
		}

	if ( Input->BoneIndices != NULL )
		{
		Output->BoneIndices = malloc ( sizeof ( uint32_t ) * Input->VertexCount * 4 );
		ASSERT_FAIL ( Output->BoneIndices != NULL );
		memcpy ( Output->BoneIndices, Input->BoneIndices, sizeof ( uint32_t ) * Input->VertexCount * 4 );
		}

	Output->VertexCount = Input->VertexCount;
	Output->IndexCount = Input->IndexCount;
	Output->BoneCount = Input->BoneCount;
	Output->Primitive = Input->Primitive;
	Output->MeshName = strdup ( Input->MeshName );
	Output->MaterialName = strdup ( Input->MaterialName );
	}

void DestroyMeshDescriptor ( MeshDescriptor *Descriptor )
	{
	SAFE_DEL_C ( Descriptor->Colors );
	SAFE_DEL_C ( Descriptor->Positions );
	SAFE_DEL_C ( Descriptor->Normals );
	SAFE_DEL_C ( Descriptor->Tangents );
	SAFE_DEL_C ( Descriptor->Bitangents );
	SAFE_DEL_C ( Descriptor->TexCoords );
	SAFE_DEL_C ( Descriptor->Indices );
	for ( unsigned BoneIndex = 0; BoneIndex < Descriptor->BoneCount; ++BoneIndex )
		{
		DestroyMeshBone ( &Descriptor->Bones[BoneIndex] );
		}
	SAFE_DEL_C ( Descriptor->Bones );
	SAFE_DEL_C ( Descriptor->BoneWeights );
	SAFE_DEL_C ( Descriptor->BoneIndices );
	SAFE_DEL_C ( Descriptor->MeshName );
	SAFE_DEL_C ( Descriptor->MaterialName );
	}

void CopyMeshTreeNode ( MeshTreeNode *Output, const MeshTreeNode *Input )
	{
	memcpy ( Output, Input, sizeof ( MeshTreeNode ) );
	Output->NodeName = strdup ( Input->NodeName );
	if ( Input->ChildNodeCount != 0 )
		{
		Output->ChildNodes = malloc ( sizeof ( int ) * Input->ChildNodeCount );
		ASSERT_FAIL ( Output->ChildNodes != NULL );
		memcpy ( Output->ChildNodes, Input->ChildNodes, sizeof ( int ) * Input->ChildNodeCount );
		}
	if ( Input->MeshIndexCount )
		{
		Output->MeshIndices = malloc ( sizeof ( uint32_t ) * Input->MeshIndexCount );
		ASSERT_FAIL ( Output->MeshIndices != NULL );
		memcpy ( Output->MeshIndices, Input->MeshIndices, sizeof ( uint32_t ) * Input->MeshIndexCount );
		}
	}

void DestroyMeshTreeNode ( MeshTreeNode *node )
	{
	SAFE_DEL_C ( node->ChildNodes );
	SAFE_DEL_C ( node->MeshIndices );
	SAFE_DEL_C ( node->NodeName );
	}

void LogMeshTreeNode ( MeshTreeNode *NodeTree, const unsigned NodeIndex, const unsigned Depth )
	{
	for ( unsigned index = 0; index < Depth; ++index )
		printf ( "  " );
	printf ( "Node '%s' (parent %d) with %u meshes and %u children\n", NodeTree[NodeIndex].NodeName, NodeTree[NodeIndex].ParentNode, NodeTree[NodeIndex].MeshIndexCount, NodeTree[NodeIndex].ChildNodeCount );
	for ( unsigned child_index = 0; child_index < NodeTree[NodeIndex].ChildNodeCount; ++child_index )
		{
		LogMeshTreeNode ( NodeTree, NodeTree[NodeIndex].ChildNodes[child_index], Depth + 1 );
		}
	}
