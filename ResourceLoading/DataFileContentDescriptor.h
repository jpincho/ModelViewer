#pragma once
#include <Platform/Platform.h>
#include "ModelDescriptor.h"
#include "MeshDescriptor.h"
#include "MaterialDescriptor.h"
#include "AnimationDescriptor.h"

#include <C_CrossRenderer/Texture.h>

struct CameraDescriptor
	{
	char *CameraName;
	vec3 Position, Up, Front;
	struct
		{
		float FOV, AspectRatio, Near, Far;
		} PerspectiveParameters;
	};

struct DataFileContentDescriptor
	{
	char *Filename;
	struct MeshDescriptor *Meshes;
	unsigned MeshCount;
	struct MaterialDescriptor *Materials;
	unsigned MaterialCount;
	crTextureDescriptor *Textures;
	unsigned TextureCount;
	struct MeshTreeNode *NodeTree;
	unsigned NodeCount;
	struct AnimationDescriptor *Animations;
	unsigned AnimationCount;
	struct CameraDescriptor *Cameras;
	unsigned CameraCount;
	};

BEGIN_C_DECLARATIONS
void DestroyDataFileContentDescriptor ( struct DataFileContentDescriptor *Descriptor );
END_C_DECLARATIONS
