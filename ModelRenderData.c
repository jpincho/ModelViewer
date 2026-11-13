#include "ModelRenderData.h"
#include "ShaderCreator/ShaderCreator.h"
#include <Platform/Logger.h>
#include <Platform/Platform.h>
#include "Utils.h"

bool BuildModelRenderData ( struct ModelRenderData *RenderData, const struct ModelData *InputModelData )
	{
	DestroyModelRenderData ( RenderData );

	// first, count how many meshes I have to draw ( even if there are multiple instances of the same )
	unsigned TotalMeshCount = 0;
	for ( unsigned NodeIndex = 0; NodeIndex < InputModelData->NodeCount; ++NodeIndex )
		TotalMeshCount += InputModelData->NodeTree[NodeIndex].MeshIndexCount;

	if ( TotalMeshCount == 0 )
		return false;

	// Ok, now allocate all the render commands to account for these meshes
	RenderData->MeshRenderDataCount = TotalMeshCount;
	RenderData->MeshRenderData = calloc ( TotalMeshCount, sizeof ( struct MeshRenderData ) );
	RenderData->NodeIndices = calloc ( TotalMeshCount, sizeof ( unsigned ) );
	RenderData->MeshIndices = calloc ( TotalMeshCount, sizeof ( unsigned ) );
	if ( ( RenderData->MeshRenderData == NULL ) || ( RenderData->NodeIndices == NULL ) || ( RenderData->MeshIndices == NULL ) )
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
	assert ( RenderData->MeshRenderDataCount == CurrentPosition );

	// Generate shaders for every mesh
	for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
		{
		RenderData->MeshRenderData[Index].OriginalMeshData = InputModelData->Meshes + RenderData->MeshIndices[Index];
		scShaderDefinition ShaderDefs = BuildShaderDefinitionForMeshData ( InputModelData->Meshes + RenderData->MeshIndices[Index] );
		ShaderDefs.LightModel = scLightModel_Off;
		crShaderHandle Shader = scAssembleShader ( ShaderDefs );
		if ( Shader == crShaderHandle_Invalid )
			{
			LOG_ERROR ( "Unable to assemble shader for mesh %u", RenderData->MeshIndices[Index] );
			goto OnError;
			}

		RenderData->NodeIndices[Index] = RenderData->NodeIndices[Index];
		crSetRenderCommandShader ( &RenderData->MeshRenderData[Index].Command, Shader );
		if ( BuildRenderCommandForMeshData ( &RenderData->MeshRenderData[Index].Command, InputModelData->Meshes + RenderData->MeshIndices[Index], Shader ) == false )
			{
			LOG_ERROR ( "Unable to build render command for mesh %u", RenderData->MeshIndices[Index] );
			goto OnError;
			}

		// If this mesh has bones, create a shader buffer for them
		if ( InputModelData->Meshes[RenderData->MeshIndices[Index]].BoneCount != 0 )
			{
			const unsigned BoneCount = ShaderDefs.BoneCount;

			crShaderBufferDescriptor Descriptor = { 0 };
			Descriptor.AccessType = crShaderBufferAccessType_Dynamic;
			Descriptor.BufferType = crShaderBufferType_Uniform;
			Descriptor.Data = NULL;
			Descriptor.DataSize = sizeof ( mat4 ) * BoneCount;
			Descriptor.Capacity = sizeof ( mat4 ) * BoneCount;
			RenderData->MeshRenderData[Index].BoneShaderBuffer = crCreateShaderBuffer ( Descriptor );
			if ( RenderData->MeshRenderData[Index].BoneShaderBuffer == crShaderBufferHandle_Invalid )
				{
				LOG_ERROR ( "Unable to create bone shader buffer for mesh %u", RenderData->MeshIndices[Index] );
				goto OnError;
				}
			RenderData->MeshRenderData[Index].BoneCount = BoneCount;

			mat4 *BoneMatrices = ( mat4* ) crMapShaderBuffer ( RenderData->MeshRenderData[Index].BoneShaderBuffer, crShaderBufferMapAccessType_WriteOnly );
			if ( BoneMatrices == NULL )
				goto OnError;
			for ( unsigned Index = 0; Index < BoneCount; ++Index )
				math_mat4_set_identity_matrix ( BoneMatrices + Index );
			crUnmapShaderBuffer ( RenderData->MeshRenderData[Index].BoneShaderBuffer );
			crSetRenderCommandUniformBlockValue ( &RenderData->MeshRenderData[Index].Command, crGetShaderUniformHandle ( Shader, "u_Bone" ), RenderData->MeshRenderData[Index].BoneShaderBuffer );
			}
		}

	RenderData->OriginalModelData = InputModelData;
	math_mat4_set_identity_matrix ( &RenderData->InstanceMatrix );
	return true;

OnError:
	DestroyModelRenderData ( RenderData );
	return false;
	}

static bool MeshIsSkinned ( const struct MeshRenderData *MeshData )
	{
	return ( MeshData->BoneCount != 0 ) && ( MeshData->OriginalMeshData != NULL ) && ( MeshData->OriginalMeshData->Bones != NULL );
	}

static void WriteCommandModelMatrix ( crRenderCommand *Command, const mat4 ModelMatrix )
	{
	crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ModelMatrix" ), ModelMatrix );

	mat3 InvertedModelMatrix;
	math_mat3_from_mat4 ( &InvertedModelMatrix, ModelMatrix );
	math_mat3_invert ( &InvertedModelMatrix, InvertedModelMatrix );
	mat3 NormalMatrix;
	math_mat3_transpose ( &NormalMatrix, InvertedModelMatrix );
	crSetRenderCommandUniformMatrix3Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_NormalMatrix" ), NormalMatrix );
	}

// Skinned draws stay on the bind-pose node. The bone palette already contains the animated bones.
// Unskinned draws follow the animated node world, which includes animated ancestors.
static void UpdateCommandModelMatrices ( struct ModelRenderData *RenderData )
	{
	if ( ( RenderData->OriginalModelData == NULL ) || ( RenderData->MeshRenderDataCount == 0 ) || ( RenderData->OriginalModelData->NodeCount == 0 ) )
		return;

	mat4 *BindWorldMatrices = alloca ( RenderData->OriginalModelData->NodeCount * sizeof ( mat4 ) );
	ComputeMeshTreeNodeWorldMatrices ( RenderData->OriginalModelData->NodeTree, RenderData->OriginalModelData->NodeCount, BindWorldMatrices );

	const bool UseAnimatedNodes = RenderData->Animator.OutputData.NodeMatrices != NULL;
	for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
		{
		const unsigned NodeIndex = RenderData->NodeIndices[Index];
		if ( NodeIndex >= RenderData->OriginalModelData->NodeCount )
			continue;

		const mat4 *NodeWorld = &BindWorldMatrices[NodeIndex];
		if ( ( UseAnimatedNodes ) && ( MeshIsSkinned ( &RenderData->MeshRenderData[Index] ) == false ) && ( NodeIndex < RenderData->Animator.InputData.NodeCount ) )
			NodeWorld = &RenderData->Animator.OutputData.NodeMatrices[NodeIndex];

		mat4 ModelMatrix;
		math_mat4_multiply ( &ModelMatrix, RenderData->InstanceMatrix, *NodeWorld );
		WriteCommandModelMatrix ( &RenderData->MeshRenderData[Index].Command, ModelMatrix );
		}
	}

void UpdateModelRenderDataWithTransform ( struct ModelRenderData *RenderData, struct BaseTransform *Transform, const float Scale )
	{
	if ( ( RenderData->OriginalModelData == NULL ) || ( RenderData->MeshRenderDataCount == 0 ) )
		return;
	if ( Transform == NULL )
		return;

	mat4 GlobalMatrix;
	GetTransformationMatrix ( Transform, &GlobalMatrix );

	mat4 ScaleMatrix;
	math_mat4_set_scale_matrix ( &ScaleMatrix, ( vec3 ) { Scale, Scale, Scale } );
	math_mat4_multiply ( &RenderData->InstanceMatrix, ScaleMatrix, GlobalMatrix );
	UpdateCommandModelMatrices ( RenderData );
	}

void UpdateModelRenderDataWithCamera ( const struct ModelRenderData *RenderData, struct CameraData *Camera )
	{
	if ( ( RenderData->OriginalModelData == NULL ) || ( RenderData->MeshRenderDataCount == 0 ) )
		return;
	if ( Camera == NULL )
		return;

	UpdateCameraMatrices ( Camera );
	for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
		{
		const unsigned NodeIndex = RenderData->NodeIndices[Index];
		crRenderCommand *Command = &RenderData->MeshRenderData[Index].Command;
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ViewMatrix" ), Camera->CachedValues.ViewMatrix );
		crSetRenderCommandUniformMatrix4Value ( Command, crGetShaderUniformHandle ( Command->Shader, "u_ProjectionMatrix" ), Camera->CachedValues.ProjectionMatrix );
		}
	}

bool SetModelRenderDataAnimation ( struct ModelRenderData *RenderData, struct AnimationDescriptor *Animation )
	{
	if ( SetupAnimator ( &RenderData->Animator, Animation, RenderData->OriginalModelData->NodeTree, RenderData->OriginalModelData->NodeCount ) == false )
		return false;
	RenderData->OriginalAnimationDescriptor = Animation;
	SetTime ( &RenderData->Animator, 0.0f );
	UpdateCommandModelMatrices ( RenderData );
	return true;
	}

void UpdateModelRenderDataWithAnimationTime ( struct ModelRenderData *RenderData, const double Time )
	{
	if ( ( RenderData->Animator.InputData.Animation == NULL ) || ( RenderData->OriginalModelData == NULL ) )
		return;
	SetTime ( &RenderData->Animator, Time );

	const struct ModelData *ModelData = RenderData->OriginalModelData;
	if ( ( ModelData == NULL ) || ( ModelData->NodeCount == 0 ) )
		return;

	mat4 *BindWorldMatrices = alloca ( ModelData->NodeCount * sizeof ( mat4 ) );
	ComputeMeshTreeNodeWorldMatrices ( ModelData->NodeTree, ModelData->NodeCount, BindWorldMatrices );

	for ( unsigned CommandIndex = 0; CommandIndex < RenderData->MeshRenderDataCount; ++CommandIndex )
		{
		struct MeshRenderData *MeshData = &RenderData->MeshRenderData[CommandIndex];
		if ( ( MeshData->BoneCount == 0 ) || ( MeshData->OriginalMeshData->Bones == NULL ) )
			continue;

		const unsigned NodeIndex = RenderData->NodeIndices[CommandIndex];
		if ( NodeIndex >= ModelData->NodeCount )
			continue;

		mat4 *BoneMatrices = crMapShaderBuffer ( MeshData->BoneShaderBuffer, crShaderBufferMapAccessType_WriteOnly );
		if ( BoneMatrices == NULL )
			{
			LOG_ERROR ( "Failed to map bone shader buffer for writing" );
			continue;
			}
		FillBoneMatricesWithAnimator ( &RenderData->Animator, BoneMatrices, MeshData->BoneCount,
		                               MeshData->OriginalMeshData->Bones, MeshData->OriginalMeshData->BoneCount,
		                               BindWorldMatrices[NodeIndex] );

		crUnmapShaderBuffer ( MeshData->BoneShaderBuffer );
		}

	UpdateCommandModelMatrices ( RenderData );
	}

void DestroyModelRenderData ( struct ModelRenderData *RenderData )
	{
	if ( RenderData->MeshRenderData != NULL )
		{
		for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
			{
			SAFE_DELETE_SHADER_BUFFER ( RenderData->MeshRenderData[Index].BoneShaderBuffer );
			SAFE_DELETE_SHADER ( RenderData->MeshRenderData[Index].Command.Shader );
			crDestroyRenderCommand ( &RenderData->MeshRenderData[Index].Command );
			}
		}
	SAFE_DEL_C ( RenderData->MeshRenderData );
	SAFE_DEL_C ( RenderData->NodeIndices );
	SAFE_DEL_C ( RenderData->MeshIndices );
	FreeAnimator ( &RenderData->Animator );
	memset ( RenderData, 0, sizeof ( struct ModelRenderData ) );
	}

void RenderModel ( const struct ModelRenderData *RenderData )
	{
	for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
		{
		crRunCommand ( RenderData->MeshRenderData[Index].Command );
		}
	}
