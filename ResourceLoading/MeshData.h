#pragma once
#include "MeshDescriptor.h"
#include "MaterialData.h"

typedef struct
	{
	crShaderBufferHandle Position, Color, Normal, TexCoord, Index, Tangent, Bitangent, BoneIndex, BoneWeight;
	crShaderBufferDataStream PositionStream, ColorStream, NormalStream, TexCoordStream, IndexStream, TangentStream, BitangentStream, BoneIndexStream, BoneWeightStream;
	crPrimitiveType Primitive;
	MaterialData Material;
	uint32_t VertexCount, BoneCount;
	} MeshData;

bool BuildMeshDataFromDescriptor ( const MeshDescriptor Descriptor, MeshData *Data );
void DestroyMeshData ( MeshData *Data );
