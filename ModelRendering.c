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
	for ( unsigned Index = 0; Index < RenderingData.ModelRenderData.MeshRenderDataCount; ++Index )
		{
		crRenderCommand *Command = &RenderingData.ModelRenderData.MeshRenderData[Index].Command;
		if ( NewOptions.Wireframe )
			Command->State.PolygonMode.State = crPolygonMode_Line;
		else
			Command->State.PolygonMode.State = crPolygonMode_Fill;
		}
	}

struct ModelRenderOptions GetModelRenderOptions ( void )
	{
	return RenderingData.Options;
	}

void UpdateRenderingForCamera ( struct CameraData *Camera )
	{
	UpdateModelRenderData ( &RenderingData.ModelRenderData, &RenderingData.BaseTransform, RenderingData.Options.Scale, Camera->CachedValues.ViewMatrix, Camera->CachedValues.ProjectionMatrix );
	}

void UpdateRenderingWithAnimationTime ( const double Time )
	{
	if ( ( RenderingData.Animator.InputData.Animation == NULL ) || ( RenderingData.ModelHandle == ResourceHandle_Invalid ) )
		return;
	SetTime ( &RenderingData.Animator, Time );

	const struct ModelData *ModelData = RenderingData.ModelRenderData.OriginalModelData;
	if ( ( ModelData == NULL ) || ( ModelData->NodeCount == 0 ) )
		return;

	mat4 *BindWorldMatrices = alloca ( ModelData->NodeCount * sizeof ( mat4 ) );
	ComputeMeshTreeNodeWorldMatrices ( ModelData->NodeTree, ModelData->NodeCount, BindWorldMatrices );

	for ( unsigned CommandIndex = 0; CommandIndex < RenderingData.ModelRenderData.MeshRenderDataCount; ++CommandIndex )
		{
		struct MeshRenderData *MeshData = &RenderingData.ModelRenderData.MeshRenderData[CommandIndex];
		if ( ( MeshData->BoneCount == 0 ) || ( MeshData->OriginalMeshData->Bones == NULL ) )
			continue;

		const unsigned NodeIndex = RenderingData.ModelRenderData.NodeIndices[CommandIndex];
		if ( NodeIndex >= ModelData->NodeCount )
			continue;

		mat4 *BoneMatrices = crMapShaderBuffer ( MeshData->BoneShaderBuffer, crShaderBufferMapAccessType_WriteOnly );
		if ( BoneMatrices == NULL )
			{
			LOG_ERROR ( "Failed to map bone shader buffer for writing" );
			return;
			}
		FillBoneMatricesWithAnimator ( &RenderingData.Animator, BoneMatrices, MeshData->BoneCount,
		                               MeshData->OriginalMeshData->Bones, MeshData->OriginalMeshData->BoneCount,
		                               BindWorldMatrices[NodeIndex] );

		crUnmapShaderBuffer ( MeshData->BoneShaderBuffer );
		}
	}

void RenderModel ( void )
	{
	for ( unsigned Index = 0; Index < RenderingData.ModelRenderData.MeshRenderDataCount; ++Index )
		{
		crRunCommand ( RenderingData.ModelRenderData.MeshRenderData[Index].Command );
		}
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
	SetTime ( &RenderingData.Animator, 0.0f );
	return true;
	}

AnimatorData *GetAnimatorForRendering ( void )
	{
	return &RenderingData.Animator;
	}
