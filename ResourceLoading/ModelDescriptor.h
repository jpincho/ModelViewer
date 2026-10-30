#pragma once
#include "MeshDescriptor.h"
#include "AABB.h"

#define MAX_WEIGHTS_PER_VERTEX 4

struct ModelDescriptor
	{
	char *Name;
	struct MeshDescriptor *Meshes;
	unsigned MeshCount;
	struct MeshTreeNode *Nodes;
	unsigned NodeCount;
	struct AABB AABBox;
	};

void LogModelDescriptor ( const struct ModelDescriptor Descriptor, const int Indentation );
void CopyModelDescriptor ( struct ModelDescriptor *Output, const struct ModelDescriptor *Input );
void DestroyModelDescriptor ( struct ModelDescriptor *Descriptor );
void ComputeMeshTreeNodeWorldMatrices ( const struct MeshTreeNode *NodeTree, const unsigned NodeCount, mat4 *WorldMatrices );
void CalculateModelDescriptorAABB ( struct ModelDescriptor *Descriptor );