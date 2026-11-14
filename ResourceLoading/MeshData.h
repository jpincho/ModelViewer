#pragma once
#include "MeshDescriptor.h"
#include "MaterialData.h"

// Structure with buffers used to render a mesh. This is one per type of mesh, not one per instance.
struct MeshData
	{
	crShaderBufferHandle Position, Color, Shininess, Emissive, Normal, TexCoord, Index, Tangent, Bitangent, BoneIndex, BoneWeight;
	crShaderBufferDataStream PositionStream, ShininessStream, EmissiveStream, ColorStream, NormalStream, TexCoordStream, IndexStream, TangentStream, BitangentStream, BoneIndexStream, BoneWeightStream;
	crPrimitiveType Primitive;
	struct MaterialData Material;
	uint32_t VertexCount;

	struct MeshBone *Bones;
	unsigned BoneCount;
	};

bool BuildMeshDataFromDescriptor ( const struct MeshDescriptor Descriptor, struct MeshData *Data );
void DestroyMeshData ( struct MeshData *Data );
