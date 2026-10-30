#pragma once
#include <Platform/Platform.h>
#include <Math/mathlib.h>
#include <C_CrossRenderer/CrossRenderer.h>
#include "AABB.h"
#include <stdint.h>

BEGIN_C_DECLARATIONS

#define MAX_WEIGHTS_PER_VERTEX 4

struct MeshBone
	{
	int NodeIndex;
	char *BoneName;
	mat4 ModelToBoneMatrix;
	};

struct MeshTreeNode
	{
	char *NodeName;
	mat4 DefaultTransformationMatrix;
	uint32_t *MeshIndices;
	unsigned MeshIndexCount;
	int *ChildNodes;
	unsigned ChildNodeCount;
	int ParentNode;
	};

struct MeshDescriptor
	{
	vec4 *Colors, *Emissive;
	vec3 *Positions, *Normals, *Tangents, *Bitangents;
	vec2 *TexCoords;
	float *Shininess;
	uint64_t VertexCount;
	uint32_t *Indices;
	uint64_t IndexCount;
	struct MeshBone *Bones;
	unsigned BoneCount;
	float *BoneWeights;
	uint32_t *BoneIndices;

	struct AABB BoundingBox;
	crPrimitiveType Primitive;
	char *MeshName, *MaterialName;
	};

void LogMeshDescriptor ( const struct MeshDescriptor Descriptor, const int Indentation );
void CopyMeshBone ( struct MeshBone *Output, const struct MeshBone *Input );
void DestroyMeshBone ( struct MeshBone *Bone );
void CopyMeshDescriptor ( struct MeshDescriptor *Output, const struct MeshDescriptor *Input );
void DestroyMeshDescriptor ( struct MeshDescriptor *Descriptor );
void CopyMeshTreeNode ( struct MeshTreeNode *Output, const struct MeshTreeNode *Input );
void DestroyMeshTreeNode ( struct MeshTreeNode *Node );
void LogMeshTreeNode ( struct MeshTreeNode *NodeTree, const unsigned NodeIndex, const unsigned Depth );
END_C_DECLARATIONS

