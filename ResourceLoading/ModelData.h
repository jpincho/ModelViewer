#pragma once
#include "ModelDescriptor.h"
#include "LightDescriptor.h"
#include "MeshData.h"

struct ModelData
	{
	struct MeshTreeNode *NodeTree;
	unsigned NodeCount;

	struct MeshData *Meshes;
	unsigned MeshCount;

	struct AABB AABBox;

	mat4 *MeshNodeTreeWorldMatrices;
	};

void DestroyModelData ( struct ModelData *Data );