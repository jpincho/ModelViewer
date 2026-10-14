#pragma once
#include <Platform/Platform.h>
#include <Math/mathlib.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include "AABB.h"
#include <stdint.h>

BEGIN_C_DECLARATIONS

#define MAX_WEIGHTS_PER_VERTEX 4

typedef struct
	{
	int NodeIndex;
	char *BoneName;
	mat4 ModelToBoneMatrix;
	} MeshBone;

typedef struct
	{
	char *NodeName;
	mat4 DefaultTransformationMatrix;
	uint32_t *MeshIndices;
	unsigned MeshIndexCount;
	int *ChildNodes;
	unsigned ChildNodeCount;
	int ParentNode;
	} MeshTreeNode;

typedef struct
	{
	vec4 *Colors;
	vec3 *Positions, *Normals, *Tangents, *Bitangents;
	vec2 *TexCoords;
	uint64_t VertexCount;
	uint32_t *Indices;
	uint64_t IndexCount;
	MeshBone *Bones;
	unsigned BoneCount;
	float *BoneWeights;
	uint32_t *BoneIndices;

	AABB BoundingBox;
	crPrimitiveType Primitive;
	char *MeshName, *MaterialName;
	} MeshDescriptor;

void LogMeshDescriptor ( const MeshDescriptor Descriptor, const int Indentation );
void CopyMeshBone ( MeshBone *Output, const MeshBone *Input );
void DestroyMeshBone ( MeshBone *Bone );
void CopyMeshDescriptor ( MeshDescriptor *Output, const MeshDescriptor *Input );
void DestroyMeshDescriptor ( MeshDescriptor *Descriptor );
void CopyMeshTreeNode ( MeshTreeNode *Output, const MeshTreeNode *Input );
void DestroyMeshTreeNode ( MeshTreeNode *Node );
void LogMeshTreeNode ( MeshTreeNode *NodeTree, const unsigned NodeIndex, const unsigned Depth );
END_C_DECLARATIONS

