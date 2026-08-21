#include "DataFileContentDescriptor.h"

void DestroyDataFileContentDescriptor ( DataFileContentDescriptor *Descriptor )
	{
	for ( unsigned index = 0; index < Descriptor->MeshCount; ++index )
		{
		DestroyMeshDescriptor ( &Descriptor->Meshes[index] );
		}
	for ( unsigned index = 0; index < Descriptor->MaterialCount; ++index )
		{
		DestroyMaterialDescriptor ( &Descriptor->Materials[index] );
		}
	for ( unsigned index = 0; index < Descriptor->TextureCount; ++index )
		{
		crDestroyTextureDescriptor ( &Descriptor->Textures[index] );
		}
	for ( unsigned index = 0; index < Descriptor->NodeCount; ++index )
		{
		DestroyMeshTreeNode ( &Descriptor->NodeTree[index] );
		}
	for ( unsigned index = 0; index < Descriptor->AnimationCount; ++index )
		{
		DestroyAnimationDescriptor ( &Descriptor->Animations[index] );
		}
	SAFE_DEL_C ( Descriptor->Meshes );
	SAFE_DEL_C ( Descriptor->Materials );
	SAFE_DEL_C ( Descriptor->Textures );
	SAFE_DEL_C ( Descriptor->Filename );
	SAFE_DEL_C ( Descriptor->NodeTree );
	SAFE_DEL_C ( Descriptor->Filename );
	}