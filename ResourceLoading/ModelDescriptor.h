#pragma once
#include "MeshDescriptor.h"
#include "BoundingBox.h"

#define MAX_WEIGHTS_PER_VERTEX 4

typedef struct
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
