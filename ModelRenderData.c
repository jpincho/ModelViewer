#include "ModelRenderData.h"
#include "ShaderCreator/ShaderCreator.h"
#include <Platform/Logger.h>
#include <Platform/Platform.h>
#include "Utils.h"

bool BuildModelRenderData ( struct ModelRenderData *RenderData, const struct ModelData *InputModelData )
	{
	memset ( RenderData, 0, sizeof ( struct ModelRenderData ) );

	// first, count how many meshes I have to draw ( even if there are multiple instances of the same )
	unsigned TotalMeshCount = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < InputModelData->NodeCount; ++NodeIndex )
		TotalMeshCount += InputModelData->NodeTree[NodeIndex].MeshIndexCount;

	if ( TotalMeshCount == 0 )
		return false;

	// Ok, now allocate all the render commands to account for these meshes
	RenderData->CommandCount = TotalMeshCount;
	RenderData->Commands = calloc ( TotalMeshCount, sizeof ( crRenderCommand ) );
	RenderData->NodeIndices = calloc ( TotalMeshCount, sizeof ( unsigned ) );
	RenderData->MeshIndices = calloc ( TotalMeshCount, sizeof ( unsigned ) );
	if ( ( RenderData->Commands == NULL ) || ( RenderData->NodeIndices == NULL ) || ( RenderData->MeshIndices == NULL ) )
		{
		DestroyModelRenderData ( RenderData );
		return false;
		}

	// Iterate again, filling the MeshIndices array, specifying which mesh each render command refers to
	unsigned CurrentPosition = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < InputModelData->NodeCount; ++NodeIndex )
		{
		for ( unsigned MeshIndex = 0; MeshIndex < InputModelData->NodeTree[NodeIndex].MeshIndexCount; ++MeshIndex )
			{
			const unsigned MeshArrayIndex = InputModelData->NodeTree[NodeIndex].MeshIndices[MeshIndex];
			if ( MeshArrayIndex >= InputModelData->MeshCount )
				{
				LOG_ERROR ( "Node '%s' references invalid mesh index %u", InputModelData->NodeTree[NodeIndex].NodeName, MeshArrayIndex );
				continue;
				}
			RenderData->MeshIndices[CurrentPosition] = MeshArrayIndex;
			RenderData->NodeIndices[CurrentPosition] = NodeIndex;
			++CurrentPosition;
			}
		}

	// These should match, at this point
	assert ( RenderData->CommandCount == CurrentPosition );

	// Generate shaders for every mesh
	for ( unsigned Index = 0; Index < RenderData->CommandCount; ++Index )
		{
		scShaderDefinition ShaderDefs = BuildShaderDefinitionForMeshData ( InputModelData->Meshes + RenderData->MeshIndices[Index] );
		ShaderDefs.LightModel = scLightModel_Off;
		crShaderHandle Shader = scAssembleShader ( ShaderDefs );
		if ( Shader == crShaderHandle_Invalid )
			{
			LOG_ERROR ( "Unable to assemble shader for mesh %u", RenderData->MeshIndices[Index] );
			goto OnError;
			}

		RenderData->NodeIndices[Index] = RenderData->NodeIndices[Index];
		crSetRenderCommandShader ( RenderData->Commands + Index, Shader );
		if ( BuildRenderCommandForMeshData ( RenderData->Commands + Index, InputModelData->Meshes + RenderData->MeshIndices[Index], Shader ) == false )
			{
			LOG_ERROR ( "Unable to build render command for mesh %u", RenderData->MeshIndices[Index] );
			goto OnError;
			}
		}

	// Instance specific data here
	// Create a buffer for the bone matrices, if necessary
	unsigned BoneCount = 0;
	for ( unsigned Index = 0; Index < TotalMeshCount; ++Index )
		{
		BoneCount += ( InputModelData->Meshes + RenderData->MeshIndices[Index] )->BoneCount;
		}
	if ( BoneCount != 0 )
		{
		unsigned FinalBoneCount = RoundUpToNearestMultiple ( BoneCount, 64 );

		crShaderBufferDescriptor Descriptor = { 0 };
		Descriptor.AccessType = crShaderBufferAccessType_Dynamic;
		Descriptor.BufferType = crShaderBufferType_Uniform;
		Descriptor.Data = NULL;
		Descriptor.DataSize = sizeof ( mat4 ) * FinalBoneCount;
		Descriptor.Capacity = sizeof ( mat4 ) * FinalBoneCount;
		RenderData->BoneShaderBuffer = crCreateShaderBuffer ( Descriptor );
		RenderData->BoneCount = BoneCount;
		RenderData->OriginalModelData = InputModelData;

		mat4 *BoneMatrices = ( mat4* ) crMapShaderBuffer ( RenderData->BoneShaderBuffer, crShaderBufferMapAccessType_WriteOnly );
		if ( BoneMatrices == NULL )
			goto OnError;
		for ( unsigned Index = 0; Index < FinalBoneCount; ++Index )
			math_mat4_set_identity_matrix ( BoneMatrices + Index );
		crUnmapShaderBuffer ( RenderData->BoneShaderBuffer );

		// Assign the bone matrices buffer to all render commands
		for ( unsigned Index = 0; Index < RenderData->CommandCount; ++Index )
			{
			crShaderHandle Shader = RenderData->Commands[Index].Shader;
			crSetRenderCommandUniformBlockValue ( RenderData->Commands + Index, crGetShaderUniformHandle ( Shader, "u_Bone" ), RenderData->BoneShaderBuffer );
			}
		}
	return true;

OnError:
	DestroyModelRenderData ( RenderData );
	return false;
	}

void UpdateModelRenderData ( const struct ModelRenderData *RenderData, struct BaseTransform *Transform, const float Scale, const mat4 ViewMatrix, const mat4 ProjectionMatrix )
	{
	mat4 GlobalMatrix;
	GetTransformationMatrix ( Transform, &GlobalMatrix );

	mat4 ScaleMatrix;
	math_mat4_set_scale_matrix ( &ScaleMatrix, ( vec3 ) { Scale, Scale, Scale } );
	mat4 ScaledGlobalMatrix;
	math_mat4_multiply ( &ScaledGlobalMatrix, ScaleMatrix, GlobalMatrix );

	mat4 *NodeWorldMatrices = alloca ( RenderData->OriginalModelData->NodeCount * sizeof ( mat4 ) );
	ComputeMeshTreeNodeWorldMatrices ( RenderData->OriginalModelData->NodeTree, RenderData->OriginalModelData->NodeCount, NodeWorldMatrices );

	for ( unsigned Index = 0; Index < RenderData->CommandCount; ++Index )
		{
		const unsigned NodeIndex = RenderData->NodeIndices[Index];
		mat4 ModelMatrix;
		math_mat4_multiply ( &ModelMatrix, ScaledGlobalMatrix, NodeWorldMatrices[NodeIndex] );

		crRenderCommand *Command = &RenderData->Commands[Index];
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ModelMatrix" ), ModelMatrix );
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ViewMatrix" ), ViewMatrix );
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ProjectionMatrix" ), ProjectionMatrix );

		mat3 InvertedModelMatrix;
		math_mat3_from_mat4 ( &InvertedModelMatrix, ModelMatrix );
		math_mat3_invert ( &InvertedModelMatrix, InvertedModelMatrix );
		mat3 NormalMatrix;
		math_mat3_transpose ( &NormalMatrix, InvertedModelMatrix );
		crSetRenderCommandUniformMatrix3Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_NormalMatrix" ), NormalMatrix );
		}
	}

void DestroyModelRenderData ( struct ModelRenderData *RenderData )
	{
	if ( RenderData->Commands != NULL )
		{
		for ( unsigned Index = 0; Index < RenderData->CommandCount; ++Index )
			{
			SAFE_DELETE_SHADER ( RenderData->Commands[Index].Shader );
			crDestroyRenderCommand ( &RenderData->Commands[Index] );
			}
		}
	SAFE_DEL_C ( RenderData->Commands );
	SAFE_DEL_C ( RenderData->NodeIndices );
	SAFE_DEL_C ( RenderData->MeshIndices );
	SAFE_DELETE_SHADER_BUFFER ( RenderData->BoneShaderBuffer );
	RenderData->BoneCount = 0;
	RenderData->CommandCount = 0;
	RenderData->OriginalModelData = NULL;
	}