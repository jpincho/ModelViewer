#include "ModelRendering.h"
#include "ModelRenderData.h"
#include <Platform/Logger.h>

static struct
	{
	struct ModelRenderOptions Options;
	struct ModelRenderData ModelRenderData;
	struct BaseTransform BaseTransform;
	ResourceHandle ModelHandle, AnimationHandle;
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
	UpdateModelRenderData ( &RenderingData.ModelRenderData, &RenderingData.BaseTransform, RenderingData.Options.Scale, Camera->CachedValues.ViewMatrix, Camera->CachedValues.ProjectionMatrix );
	}

void RenderModel ( void )
	{
	/*


		//if ( ModelCommands.BoneCount != 0 )
		//	SetTime ( &Animator, TotalDelta );
	mat4 *NodeWorldMatrices = NULL;
	if ( ( RenderingData.ModelRenderData.BoneCount != 0 ) && ( RenderingData.ModelRenderData.OriginalModelData != NULL ) )
		{
		NodeWorldMatrices = alloca ( RenderingData.ModelRenderData.OriginalModelData->NodeCount * sizeof ( mat4 ) );
		ComputeMeshTreeNodeWorldMatrices ( RenderingData.ModelRenderData.OriginalModelData->NodeTree, RenderingData.ModelRenderData.OriginalModelData->NodeCount, NodeWorldMatrices );
		}*/
	for ( unsigned Index = 0; Index < RenderingData.ModelRenderData.CommandCount; ++Index )
		{
		/*const unsigned MeshArrayIndex = RenderingData.ModelRenderData.MeshIndices[Index];
		const struct MeshDescriptor *Mesh = &Descriptor->Meshes[MeshArrayIndex];
		if ( ( Mesh->BoneCount != 0 ) && ( NodeWorldMatrices != NULL ) )
			{
			mat4 MeshWorldInverse;
			if ( math_mat4_inverse ( &MeshWorldInverse, NodeWorldMatrices[RenderingData.ModelRenderData.NodeIndices[Index]] ) == false )
				math_mat4_set_identity_matrix ( &MeshWorldInverse );
			mat4 *MappedPointer = ( mat4* ) crMapShaderBuffer ( RenderingData.ModelRenderData.BoneShaderBuffer, crShaderBufferMapAccessType_WriteOnly );
			if ( MappedPointer == NULL )
				return -1;
			// Each mesh's vertex indices start at bone 0, so this palette replaces the previous draw's.
			//FillBoneMatricesWithAnimator ( &Animator, MappedPointer, ModelCommands.BoneCount, Mesh->Bones, Mesh->BoneCount, MeshWorldInverse );
			crUnmapShaderBuffer ( RenderingData.ModelRenderData.BoneShaderBuffer );
			}*/
		crRunCommand ( RenderingData.ModelRenderData.Commands[Index] );
		}
	}


void FreeRenderingData ( void )
	{
	DestroyModelRenderData ( &RenderingData.ModelRenderData );
	math_vec3_set ( &RenderingData.BaseTransform.Position, 0.0f, 0.0f, 0.0f );
	RenderingData.BaseTransform.Orientation = MATH_QUAT_IDENTITY;
	UpdateCachedValues ( &RenderingData.BaseTransform );
	RenderingData.ModelHandle = ResourceHandle_Invalid;
	}