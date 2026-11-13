#pragma once
#include "ResourceLoading/AnimationDescriptor.h"
#include "ResourceLoading/MeshDescriptor.h"

struct AnimatorData
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
	};

bool IsAnimationCompatibleWithNodeTree ( const struct AnimationDescriptor *Animation, const struct MeshTreeNode *Nodes, const unsigned NodeCount );
bool SetupAnimator ( struct AnimatorData *Animator, const struct AnimationDescriptor *Animation, const struct MeshTreeNode *NodeTree, const unsigned NodeCount );
void SetTime ( struct AnimatorData *Animator, const float Time );
void FreeAnimator ( struct AnimatorData *Animator );
unsigned FillBoneMatricesWithAnimator ( const struct AnimatorData *Animator, mat4 *BoneMatrices, const unsigned BoneMatrixCount, const struct MeshBone *Bones, const unsigned BoneCount, const mat4 MeshWorld );