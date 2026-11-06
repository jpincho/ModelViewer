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

	double CurrentTime;
	} AnimatorData;

bool IsAnimationCompatibleWithNodeTree ( const struct AnimationDescriptor *Animation, const struct MeshTreeNode *Nodes, const unsigned NodeCount );
bool SetupAnimator ( AnimatorData *Animator, const struct AnimationDescriptor *Animation, const struct MeshTreeNode *NodeTree, const unsigned NodeCount );
void SetTime ( AnimatorData *Animator, const double Time );
void FreeAnimator ( AnimatorData *Animator );
unsigned FillBoneMatricesWithAnimator ( const AnimatorData const *Animator, mat4 *BoneMatrices, const unsigned BoneMatrixCount, const struct MeshBone const *Bones, const unsigned BoneCount, const mat4 MeshWorldInverse );