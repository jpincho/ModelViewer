#include "MeshData.h"
#include <C_CrossRenderer/CrossRenderer.h>

bool BuildMeshDataFromDescriptor ( const MeshDescriptor Descriptor, MeshData *Data )
	{
	Data->BoneCount = Descriptor.BoneCount;
	Data->Primitive = crTriangleList;

	if ( Descriptor.IndexCount != 0 )
		Data->VertexCount = Descriptor.IndexCount;
	else if ( Descriptor.VertexCount != 0 )
		Data->VertexCount = Descriptor.VertexCount;
	else
		return false;

	crShaderBufferDescriptor PositionBufferDescriptor = { 0 };
	PositionBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
	PositionBufferDescriptor.BufferType = crShaderBufferType_Array;
	PositionBufferDescriptor.Capacity = PositionBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( vec3 );
	PositionBufferDescriptor.Data = Descriptor.Positions;
	Data->Position = crCreateShaderBuffer ( PositionBufferDescriptor );
	if ( Data->Position == crShaderBufferHandle_Invalid )
		return false;

	if ( Descriptor.TexCoords )
		{
		crShaderBufferDescriptor TexCoordBufferDescriptor = { 0 };
		TexCoordBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		TexCoordBufferDescriptor.BufferType = crShaderBufferType_Array;
		TexCoordBufferDescriptor.Capacity = TexCoordBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( vec2 );
		TexCoordBufferDescriptor.Data = Descriptor.TexCoords;
		Data->TexCoord = crCreateShaderBuffer ( TexCoordBufferDescriptor );
		}
	else
		Data->TexCoord = crShaderBufferHandle_Invalid;

	if ( Descriptor.Normals )
		{
		crShaderBufferDescriptor NormalBufferDescriptor = { 0 };
		NormalBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		NormalBufferDescriptor.BufferType = crShaderBufferType_Array;
		NormalBufferDescriptor.Capacity = NormalBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( vec3 );
		NormalBufferDescriptor.Data = Descriptor.Normals;
		Data->Normal = crCreateShaderBuffer ( NormalBufferDescriptor );
		}
	else
		Data->Normal = crShaderBufferHandle_Invalid;

	if ( Descriptor.Colors )
		{
		crShaderBufferDescriptor ColorBufferDescriptor = { 0 };
		ColorBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		ColorBufferDescriptor.BufferType = crShaderBufferType_Array;
		ColorBufferDescriptor.Capacity = ColorBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( vec4 );
		ColorBufferDescriptor.Data = Descriptor.Colors;
		Data->Color = crCreateShaderBuffer ( ColorBufferDescriptor );
		}
	else
		Data->Color = crShaderBufferHandle_Invalid;

	if ( Descriptor.Shininess )
		{
		crShaderBufferDescriptor ShininessBufferDescriptor = { 0 };
		ShininessBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		ShininessBufferDescriptor.BufferType = crShaderBufferType_Array;
		ShininessBufferDescriptor.Capacity = ShininessBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( float );
		ShininessBufferDescriptor.Data = Descriptor.Shininess;
		Data->Shininess = crCreateShaderBuffer ( ShininessBufferDescriptor );
		}
	else
		Data->Shininess = crShaderBufferHandle_Invalid;

	if ( Descriptor.Emissive )
		{
		crShaderBufferDescriptor EmissiveBufferDescriptor = { 0 };
		EmissiveBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		EmissiveBufferDescriptor.BufferType = crShaderBufferType_Array;
		EmissiveBufferDescriptor.Capacity = EmissiveBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( vec4 );
		EmissiveBufferDescriptor.Data = Descriptor.Emissive;
		Data->Emissive = crCreateShaderBuffer ( EmissiveBufferDescriptor );
		}
	else
		Data->Emissive = crShaderBufferHandle_Invalid;

	if ( Descriptor.Indices )
		{
		crShaderBufferDescriptor IndexBufferDescriptor = { 0 };
		IndexBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		IndexBufferDescriptor.BufferType = crShaderBufferType_Array;
		IndexBufferDescriptor.Capacity = IndexBufferDescriptor.DataSize = Descriptor.IndexCount * sizeof ( uint32_t );
		IndexBufferDescriptor.Data = Descriptor.Indices;
		Data->Index = crCreateShaderBuffer ( IndexBufferDescriptor );
		}
	else
		Data->Index = crShaderBufferHandle_Invalid;

	if ( Descriptor.Tangents )
		{
		crShaderBufferDescriptor TangentBufferDescriptor = { 0 };
		TangentBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		TangentBufferDescriptor.BufferType = crShaderBufferType_Array;
		TangentBufferDescriptor.Capacity = TangentBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( vec3 );
		TangentBufferDescriptor.Data = Descriptor.Tangents;
		Data->Tangent = crCreateShaderBuffer ( TangentBufferDescriptor );
		}
	else
		Data->Tangent = crShaderBufferHandle_Invalid;

	if ( Descriptor.Bitangents )
		{
		crShaderBufferDescriptor BitangentBufferDescriptor = { 0 };
		BitangentBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		BitangentBufferDescriptor.BufferType = crShaderBufferType_Array;
		BitangentBufferDescriptor.Capacity = BitangentBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( vec3 );
		BitangentBufferDescriptor.Data = Descriptor.Bitangents;
		Data->Bitangent = crCreateShaderBuffer ( BitangentBufferDescriptor );
		}
	else
		Data->Bitangent = crShaderBufferHandle_Invalid;

	if ( Descriptor.BoneIndices )
		{
		crShaderBufferDescriptor BoneIndicesBufferDescriptor = { 0 };
		BoneIndicesBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		BoneIndicesBufferDescriptor.BufferType = crShaderBufferType_Array;
		BoneIndicesBufferDescriptor.Capacity = BoneIndicesBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( uint32_t ) * MAX_WEIGHTS_PER_VERTEX;
		BoneIndicesBufferDescriptor.Data = Descriptor.BoneIndices;
		Data->BoneIndex = crCreateShaderBuffer ( BoneIndicesBufferDescriptor );
		}
	else
		Data->BoneIndex = crShaderBufferHandle_Invalid;

	if ( Descriptor.BoneWeights )
		{
		crShaderBufferDescriptor BoneWeightBufferDescriptor = { 0 };
		BoneWeightBufferDescriptor.AccessType = crShaderBufferAccessType_Static;
		BoneWeightBufferDescriptor.BufferType = crShaderBufferType_Array;
		BoneWeightBufferDescriptor.Capacity = BoneWeightBufferDescriptor.DataSize = Descriptor.VertexCount * sizeof ( float ) * MAX_WEIGHTS_PER_VERTEX;
		BoneWeightBufferDescriptor.Data = Descriptor.BoneWeights;
		Data->BoneWeight = crCreateShaderBuffer ( BoneWeightBufferDescriptor );
		}
	else
		Data->BoneWeight = crShaderBufferHandle_Invalid;


	// Create the streams, if not previously created
	if ( ( !Data->PositionStream.BufferHandle ) && ( Data->Position ) )
		{
		Data->PositionStream.BufferHandle = Data->Position;
		Data->PositionStream.ComponentsPerElement = 3;
		Data->PositionStream.ComponentType = crShaderBufferComponentType_Float;
		Data->PositionStream.NormalizeData = false;
		Data->PositionStream.PerInstance = false;
		Data->PositionStream.StartOffset = 0;
		Data->PositionStream.Stride = sizeof ( vec3 );
		}
	if ( ( !Data->ColorStream.BufferHandle ) && ( Data->Color ) )
		{
		Data->ColorStream.BufferHandle = Data->Color;
		Data->ColorStream.ComponentsPerElement = 4;
		Data->ColorStream.ComponentType = crShaderBufferComponentType_Float;
		Data->ColorStream.NormalizeData = false;
		Data->ColorStream.PerInstance = false;
		Data->ColorStream.StartOffset = 0;
		Data->ColorStream.Stride = sizeof ( vec4 );
		}
	if ( ( !Data->ShininessStream.BufferHandle ) && ( Data->Shininess ) )
		{
		Data->ShininessStream.BufferHandle = Data->Shininess;
		Data->ShininessStream.ComponentsPerElement = 1;
		Data->ShininessStream.ComponentType = crShaderBufferComponentType_Float;
		Data->ShininessStream.NormalizeData = false;
		Data->ShininessStream.PerInstance = false;
		Data->ShininessStream.StartOffset = 0;
		Data->ShininessStream.Stride = sizeof ( float );
		}
	if ( ( !Data->EmissiveStream.BufferHandle ) && ( Data->Emissive ) )
		{
		Data->EmissiveStream.BufferHandle = Data->Emissive;
		Data->EmissiveStream.ComponentsPerElement = 4;
		Data->EmissiveStream.ComponentType = crShaderBufferComponentType_Float;
		Data->EmissiveStream.NormalizeData = false;
		Data->EmissiveStream.PerInstance = false;
		Data->EmissiveStream.StartOffset = 0;
		Data->EmissiveStream.Stride = sizeof ( vec4 );
		}
	if ( ( !Data->TexCoordStream.BufferHandle ) && ( Data->TexCoord ) )
		{
		Data->TexCoordStream.BufferHandle = Data->TexCoord;
		Data->TexCoordStream.ComponentsPerElement = 2;
		Data->TexCoordStream.ComponentType = crShaderBufferComponentType_Float;
		Data->TexCoordStream.NormalizeData = false;
		Data->TexCoordStream.PerInstance = false;
		Data->TexCoordStream.StartOffset = 0;
		Data->TexCoordStream.Stride = sizeof ( vec2 );
		}
	if ( ( !Data->NormalStream.BufferHandle ) && ( Data->Normal ) )
		{
		Data->NormalStream.BufferHandle = Data->Normal;
		Data->NormalStream.ComponentsPerElement = 3;
		Data->NormalStream.ComponentType = crShaderBufferComponentType_Float;
		Data->NormalStream.NormalizeData = false;
		Data->NormalStream.PerInstance = false;
		Data->NormalStream.StartOffset = 0;
		Data->NormalStream.Stride = sizeof ( vec3 );
		}
	if ( ( !Data->TangentStream.BufferHandle ) && ( Data->Tangent ) )
		{
		Data->TangentStream.BufferHandle = Data->Tangent;
		Data->TangentStream.ComponentsPerElement = 3;
		Data->TangentStream.ComponentType = crShaderBufferComponentType_Float;
		Data->TangentStream.NormalizeData = false;
		Data->TangentStream.PerInstance = false;
		Data->TangentStream.StartOffset = 0;
		Data->TangentStream.Stride = sizeof ( vec3 );
		}
	if ( ( !Data->BitangentStream.BufferHandle ) && ( Data->Bitangent ) )
		{
		Data->BitangentStream.BufferHandle = Data->Bitangent;
		Data->BitangentStream.ComponentsPerElement = 3;
		Data->BitangentStream.ComponentType = crShaderBufferComponentType_Float;
		Data->BitangentStream.NormalizeData = false;
		Data->BitangentStream.PerInstance = false;
		Data->BitangentStream.StartOffset = 0;
		Data->BitangentStream.Stride = sizeof ( vec3 );
		}
	if ( ( !Data->IndexStream.BufferHandle ) && ( Data->Index ) )
		{
		Data->IndexStream.BufferHandle = Data->Index;
		Data->IndexStream.ComponentsPerElement = 1;
		Data->IndexStream.ComponentType = crShaderBufferComponentType_Unsigned32;
		Data->IndexStream.NormalizeData = false;
		Data->IndexStream.PerInstance = false;
		Data->IndexStream.StartOffset = 0;
		Data->IndexStream.Stride = sizeof ( uint32_t );
		}
	if ( ( !Data->BoneIndexStream.BufferHandle ) && ( Data->BoneIndex ) )
		{
		Data->BoneIndexStream.BufferHandle = Data->BoneIndex;
		Data->BoneIndexStream.ComponentsPerElement = 4;
		Data->BoneIndexStream.ComponentType = crShaderBufferComponentType_Unsigned32;
		Data->BoneIndexStream.NormalizeData = false;
		Data->BoneIndexStream.PerInstance = false;
		Data->BoneIndexStream.StartOffset = 0;
		Data->BoneIndexStream.Stride = sizeof ( uint32_t ) * MAX_WEIGHTS_PER_VERTEX;
		}
	if ( ( !Data->BoneWeightStream.BufferHandle ) && ( Data->BoneWeight ) )
		{
		Data->BoneWeightStream.BufferHandle = Data->BoneWeight;
		Data->BoneWeightStream.ComponentsPerElement = 4;
		Data->BoneWeightStream.ComponentType = crShaderBufferComponentType_Float;
		Data->BoneWeightStream.NormalizeData = false;
		Data->BoneWeightStream.PerInstance = false;
		Data->BoneWeightStream.StartOffset = 0;
		Data->BoneWeightStream.Stride = sizeof ( vec4 );
		}

	return true;
	}

void DestroyMeshData ( MeshData *Data )
	{
	SAFE_DELETE_SHADER_BUFFER ( Data->Bitangent );
	SAFE_DELETE_SHADER_BUFFER ( Data->BoneIndex );
	SAFE_DELETE_SHADER_BUFFER ( Data->BoneWeight );
	SAFE_DELETE_SHADER_BUFFER ( Data->Color );
	SAFE_DELETE_SHADER_BUFFER ( Data->Emissive );
	SAFE_DELETE_SHADER_BUFFER ( Data->Shininess );
	SAFE_DELETE_SHADER_BUFFER ( Data->Index );
	SAFE_DELETE_SHADER_BUFFER ( Data->Normal );
	SAFE_DELETE_SHADER_BUFFER ( Data->Position );
	SAFE_DELETE_SHADER_BUFFER ( Data->Tangent );
	SAFE_DELETE_SHADER_BUFFER ( Data->TexCoord );
	}
