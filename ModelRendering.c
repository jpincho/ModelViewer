#include "ModelRendering.h"
#include "ModelRenderData.h"
#include <Platform/Logger.h>
#include "Animator.h"
#include <alloca.h>

static struct
	{
	struct ModelRenderOptions Options;
	struct ModelRenderData ModelRenderData;
	struct BaseTransform BaseTransform;
	ResourceHandle ModelHandle, AnimationHandle;
	AnimatorData Animator;
	} RenderingData = {0};

bool SetModelToRender ( const ResourceHandle NewModelHandle )
	{
	FreeRenderingData();
	if ( NewModelHandle == ResourceHandle_Invalid )
		return true;
	struct ModelData *NewModelData = ResourceHub_GetModelData ( NewModelHandle );
	if ( NewModelData == NULL )
		return false;
	if ( BuildModelRenderData ( &RenderingData.ModelRenderData, NewModelData ) == false )
		return false;
	RenderingData.ModelHandle = NewModelHandle;

	return true;
	}

void SetModelRenderOptions ( const struct ModelRenderOptions NewOptions )
	{
	RenderingData.Options = NewOptions;
	}

struct ModelRenderOptions GetModelRenderOptions ( void )
	{
	return RenderingData.Options;
	}

void UpdateRenderingForCamera ( struct CameraData *Camera )
	{
	if ( RenderingData.ModelRenderData.OriginalModelData == NULL )
		return;

	const mat4 *AnimatedNodeWorlds = NULL;
	if ( ( RenderingData.Animator.InputData.Animation != NULL ) &&
	        ( RenderingData.Animator.OutputData.NodeMatrices != NULL ) &&
	        ( RenderingData.Animator.InputData.NodeCount == RenderingData.ModelRenderData.OriginalModelData->NodeCount ) )
		AnimatedNodeWorlds = RenderingData.Animator.OutputData.NodeMatrices;
	UpdateModelRenderData ( &RenderingData.ModelRenderData, &RenderingData.BaseTransform, RenderingData.Options.Scale, Camera->CachedValues.ViewMatrix, Camera->CachedValues.ProjectionMatrix, AnimatedNodeWorlds );
	}

void UpdateRenderingWithAnimationTime ( const double Time )
	{
	SetTime ( &RenderingData.Animator, Time );
	}

void RenderModel ( void )
	{
	const struct ModelRenderData *RenderData = &RenderingData.ModelRenderData;
	const struct ModelDescriptor *Descriptor = ResourceHub_GetModelDescriptor ( RenderingData.ModelHandle );
#if 0
	mat4 *NodeWorldMatrices = NULL;
	const bool UploadBones = ( RenderData->BoneCount != 0 ) &&
	                         ( RenderData->BoneShaderBuffer != crShaderBufferHandle_Invalid ) &&
	                         ( RenderData->OriginalModelData != NULL ) &&
	                         ( RenderData->OriginalModelData->NodeCount != 0 ) &&
	                         ( Descriptor != NULL ) &&
	                         ( RenderingData.Animator.OutputData.NodeMatrices != NULL );
	if ( UploadBones )
		{
		NodeWorldMatrices = alloca ( RenderData->OriginalModelData->NodeCount * sizeof ( mat4 ) );
		ComputeMeshTreeNodeWorldMatrices ( RenderData->OriginalModelData->NodeTree, RenderData->OriginalModelData->NodeCount, NodeWorldMatrices );
		}

	for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
		{
		/* Vertex bone indices are local to each mesh and start at 0, so the shared palette is replaced before every draw. */
		if ( UploadBones )
			{
			const unsigned MeshArrayIndex = RenderData->MeshIndices[Index];
			if ( MeshArrayIndex < Descriptor->MeshCount )
				{
				const struct MeshDescriptor *Mesh = &Descriptor->Meshes[MeshArrayIndex];
				if ( ( Mesh->BoneCount != 0 ) && ( Mesh->Bones != NULL ) )
					{
					mat4 MeshWorldInverse;
					if ( math_mat4_inverse ( &MeshWorldInverse, NodeWorldMatrices[RenderData->NodeIndices[Index]] ) == false )
						math_mat4_set_identity_matrix ( &MeshWorldInverse );
					mat4 *MappedPointer = ( mat4 * ) crMapShaderBuffer ( RenderData->BoneShaderBuffer, crShaderBufferMapAccessType_WriteOnly );
					if ( MappedPointer != NULL )
						{
						FillBoneMatricesWithAnimator ( &RenderingData.Animator, MappedPointer, Mesh->BoneCount, Mesh->Bones, Mesh->BoneCount, MeshWorldInverse );
						crUnmapShaderBuffer ( RenderData->BoneShaderBuffer );
						}
					}
				}
			}
		crRunCommand ( RenderData->MeshRenderData[Index].Command );
		}
#else
	mat4 *NodeWorldMatrices = NULL;
	bool HasBones = false;
	for ( unsigned MeshIndex = 0; MeshIndex < RenderData->MeshRenderDataCount; ++MeshIndex )
		{
		if ( RenderData->MeshRenderData[MeshIndex].BoneCount != 0 )
			{
			HasBones = true;
			break;
			}
		}
	if ( ( HasBones == true ) && ( RenderData->OriginalModelData != NULL ) && ( Descriptor != NULL ) )
		{
		NodeWorldMatrices = alloca ( RenderData->OriginalModelData->NodeCount * sizeof ( mat4 ) );
		ComputeMeshTreeNodeWorldMatrices ( RenderData->OriginalModelData->NodeTree, RenderData->OriginalModelData->NodeCount, NodeWorldMatrices );
		}
	else
		HasBones = false;
	for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
		{
		if ( ( HasBones == true ) && ( RenderData->MeshRenderData[Index].BoneCount != 0 ) )
			{
			const unsigned MeshArrayIndex = RenderData->MeshIndices[Index];
			if ( ( MeshArrayIndex < Descriptor->MeshCount ) && ( Descriptor->Meshes[MeshArrayIndex].Bones != NULL ) )
				{
				const struct MeshDescriptor *MeshDescriptor = &Descriptor->Meshes[MeshArrayIndex];
				mat4 MeshWorldInverse;
				if ( math_mat4_inverse ( &MeshWorldInverse, NodeWorldMatrices[RenderData->NodeIndices[Index]] ) == false )
					math_mat4_set_identity_matrix ( &MeshWorldInverse );
				mat4 *MappedPointer = ( mat4 * ) crMapShaderBuffer ( RenderData->MeshRenderData[Index].BoneShaderBuffer, crShaderBufferMapAccessType_WriteOnly );
				if ( MappedPointer != NULL )
					{
					FillBoneMatricesWithAnimator ( &RenderingData.Animator, MappedPointer, RenderData->MeshRenderData[Index].BoneCount, MeshDescriptor->Bones, MeshDescriptor->BoneCount, MeshWorldInverse );
					crUnmapShaderBuffer ( RenderData->MeshRenderData[Index].BoneShaderBuffer );
					}
				}
			}
		}

	for ( unsigned Index = 0; Index < RenderData->MeshRenderDataCount; ++Index )
		{
		crRunCommand ( RenderData->MeshRenderData[Index].Command );
		}
#endif
	}

void FreeRenderingData ( void )
	{
	DestroyModelRenderData ( &RenderingData.ModelRenderData );
	math_vec3_set ( &RenderingData.BaseTransform.Position, 0.0f, 0.0f, 0.0f );
	RenderingData.BaseTransform.Orientation = MATH_QUAT_IDENTITY;
	UpdateCachedValues ( &RenderingData.BaseTransform );
	FreeAnimator ( &RenderingData.Animator );
	RenderingData.ModelHandle = ResourceHandle_Invalid;
	RenderingData.AnimationHandle = ResourceHandle_Invalid;
	}

bool SetAnimationToRender ( const ResourceHandle NewAnimationHandle )
	{
	const struct AnimationDescriptor *AnimationDescriptor = ResourceHub_GetAnimationDescriptor ( NewAnimationHandle );
	if ( ( AnimationDescriptor == NULL ) || ( RenderingData.ModelRenderData.OriginalModelData == NULL ) )
		return false;
	if ( SetupAnimator ( &RenderingData.Animator, AnimationDescriptor, RenderingData.ModelRenderData.OriginalModelData->NodeTree, RenderingData.ModelRenderData.OriginalModelData->NodeCount ) == false )
		return false;
	RenderingData.AnimationHandle = NewAnimationHandle;
	return true;
	}

AnimatorData *GetAnimatorForRendering ( void )
	{
	return &RenderingData.Animator;
	}