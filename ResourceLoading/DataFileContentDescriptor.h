#pragma once
#include <Platform/defines.h>
#include "ModelDescriptor.h"
#include "MeshDescriptor.h"
#include "MaterialDescriptor.h"
#include "AnimationDescriptor.h"
#include <C_CrossRenderer/Texture.h>

typedef struct
	{
	char *Filename;
	MeshDescriptor *Meshes;
	unsigned MeshCount;
	MaterialDescriptor *Materials;
	unsigned MaterialCount;
	crTextureDescriptor *Textures;
	unsigned TextureCount;
	MeshTreeNode *NodeTree;
	unsigned NodeCount;
	AnimationDescriptor *Animations;
	unsigned AnimationCount;
	} DataFileContentDescriptor;

BEGIN_C_DECLARATIONS
void DestroyDataFileContentDescriptor ( DataFileContentDescriptor *Descriptor );
END_C_DECLARATIONS
