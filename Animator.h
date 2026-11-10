#pragma once
#include "ResourceLoading/AnimationDescriptor.h"
#include "ResourceLoading/MeshDescriptor.h"

typedef struct
	{
	struct
		{
		const struct AnimationDescriptor *Animation;
		const struct MeshTreeNode *NodeTree;
		unsigned NodeCount;
		} InputData;

	struct
		{
		int *NodeToTrack;
		mat4 *NodeMatrices;
		} OutputData;

	float CurrentTime;
	} AnimatorData;

bool IsAnimationCompatibleWithNodeTree ( const struct AnimationDescriptor *Animation, const struct MeshTreeNode *Nodes, const unsigned NodeCount );
bool SetupAnimator ( AnimatorData *Animator, const struct AnimationDescriptor *Animation, const struct MeshTreeNode *NodeTree, const unsigned NodeCount );
void SetTime ( AnimatorData *Animator, const float Time );
void FreeAnimator ( AnimatorData *Animator );
unsigned FillBoneMatricesWithAnimator ( const AnimatorData *Animator, mat4 *BoneMatrices, const unsigned BoneMatrixCount, const struct MeshBone *Bones, const unsigned BoneCount, const mat4 MeshWorld );