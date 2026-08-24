#pragma once
#include "ModelDescriptor.h"
#include "LightDescriptor.h"
#include "MeshData.h"

typedef struct
	{
	MeshTreeNode *NodeTree;
	unsigned NodeCount;

	MeshData *Meshes;
	unsigned MeshCount;
	} ModelData;

void DestroyModelData ( ModelData *Data );