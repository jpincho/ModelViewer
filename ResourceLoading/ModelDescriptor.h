#pragma once
#include "MeshDescriptor.h"
#include "AABB.h"

#define MAX_WEIGHTS_PER_VERTEX 4

typedef struct ModelDescriptor
	{
	char *Name;
	MeshDescriptor *Meshes;
	unsigned MeshCount;
	MeshTreeNode *Nodes;
	unsigned NodeCount;
	AABB AABBox;
	} ModelDescriptor;

void LogModelDescriptor ( const ModelDescriptor Descriptor, const int Indentation );
void CopyModelDescriptor ( ModelDescriptor *Output, const ModelDescriptor *Input );
void DestroyModelDescriptor ( ModelDescriptor *Descriptor );
