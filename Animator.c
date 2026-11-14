#include "Animator.h"
#include <Platform/Platform.h>
#include <Platform/PlatformTime.h>
#include <Platform/Logger.h>
#include <string.h>

static int FindTrackIndexInAnimation ( const struct AnimationDescriptor *Animation, const char *NodeName )
	{
	for ( unsigned Index = 0; Index < Animation->NodeTrackCount; ++Index )
		{
		if ( strcmp ( Animation->NodeTracks[Index].NodeName, NodeName ) == 0 )
			{
			return Index;
			}
		}
	return -1;
	}

// Index of the first key strictly after TimeStamp. KeyCount means the timestamp is at or past the last key.
static unsigned FindNodeTrackKeyForTime ( const NodeTrackKey *Keys, const unsigned KeyCount, const float TimeStamp )
	{
	for ( unsigned KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex )
		{
		if ( Keys[KeyIndex].Time > TimeStamp )
			return KeyIndex;
		}
	return KeyCount;
	}

static vec3 GetInterpolatedPosition ( const NodeTrack AnimationTrack, const float TimeStamp )
	{
	if ( AnimationTrack.PositionCount == 0 )
		return MATH_VEC3_ZERO;
	if ( AnimationTrack.PositionCount == 1 )
		return AnimationTrack.Positions[0].Vec3Value;

	unsigned NextKeyIndex = FindNodeTrackKeyForTime ( AnimationTrack.Positions, AnimationTrack.PositionCount, TimeStamp );
	if ( NextKeyIndex == 0 )
		return AnimationTrack.Positions[0].Vec3Value;
	if ( NextKeyIndex >= AnimationTrack.PositionCount )
		return AnimationTrack.Positions[AnimationTrack.PositionCount - 1].Vec3Value;

	// Interpolate between this value and the next
	double TimeBetweenKeys = AnimationTrack.Positions[NextKeyIndex].Time - AnimationTrack.Positions[NextKeyIndex - 1].Time;
	double Factor = ( TimeStamp - AnimationTrack.Positions[NextKeyIndex - 1].Time ) / TimeBetweenKeys;

	vec3 FinalPosition;
	math_vec3_lerp ( &FinalPosition, AnimationTrack.Positions[NextKeyIndex - 1].Vec3Value, AnimationTrack.Positions[NextKeyIndex].Vec3Value, Factor );
	return FinalPosition;
	}

static quat GetInterpolatedRotation ( const NodeTrack AnimationTrack, const float TimeStamp )
	{
	if ( AnimationTrack.RotationCount == 0 )
		return MATH_QUAT_IDENTITY;
	if ( AnimationTrack.RotationCount == 1 )
		return AnimationTrack.Rotations[0].QuatValue;

	unsigned NextKeyIndex = FindNodeTrackKeyForTime ( AnimationTrack.Rotations, AnimationTrack.RotationCount, TimeStamp );
	if ( NextKeyIndex == 0 )
		return AnimationTrack.Rotations[0].QuatValue;
	if ( NextKeyIndex >= AnimationTrack.RotationCount )
		return AnimationTrack.Rotations[AnimationTrack.RotationCount - 1].QuatValue;

	// Interpolate between this value and the next
	double TimeBetweenKeys = AnimationTrack.Rotations[NextKeyIndex].Time - AnimationTrack.Rotations[NextKeyIndex - 1].Time;
	double Factor = ( TimeStamp - AnimationTrack.Rotations[NextKeyIndex - 1].Time ) / TimeBetweenKeys;

	quat FinalRotation;
	math_quat_slerp ( &FinalRotation, AnimationTrack.Rotations[NextKeyIndex - 1].QuatValue, AnimationTrack.Rotations[NextKeyIndex].QuatValue, Factor );
	return FinalRotation;
	}

static vec3 GetInterpolatedScale ( const NodeTrack AnimationTrack, const float TimeStamp )
	{
	if ( AnimationTrack.ScaleCount == 0 )
		return MATH_VEC3_ONE;
	if ( AnimationTrack.ScaleCount == 1 )
		return AnimationTrack.Scales[0].Vec3Value;

	unsigned NextKeyIndex = FindNodeTrackKeyForTime ( AnimationTrack.Scales, AnimationTrack.ScaleCount, TimeStamp );
	if ( NextKeyIndex == 0 )
		return AnimationTrack.Scales[0].Vec3Value;
	if ( NextKeyIndex >= AnimationTrack.ScaleCount )
		return AnimationTrack.Scales[AnimationTrack.ScaleCount - 1].Vec3Value;

	// Interpolate between this value and the next
	double TimeBetweenKeys = AnimationTrack.Scales[NextKeyIndex].Time - AnimationTrack.Scales[NextKeyIndex - 1].Time;
	double Factor = ( TimeStamp - AnimationTrack.Scales[NextKeyIndex - 1].Time ) / TimeBetweenKeys;

	vec3 FinalScale;
	math_vec3_lerp ( &FinalScale, AnimationTrack.Scales[NextKeyIndex - 1].Vec3Value, AnimationTrack.Scales[NextKeyIndex].Vec3Value, Factor );
	return FinalScale;
	}

static void ApplyAnimationToNode ( struct AnimatorData *Animator, const float Ticks, const unsigned NodeIndex, const mat4 ParentTransform )
	{
	mat4 CurrentNodeTransform;

	// See if this node has a track in this animation. If so, animate it
	int NodeTrackIndex = Animator->OutputData.NodeToTrack[NodeIndex];
	if ( NodeTrackIndex != -1 )
		{
		vec3 Position = GetInterpolatedPosition ( Animator->InputData.Animation->NodeTracks[NodeTrackIndex], Ticks );
		vec3 Scale = GetInterpolatedScale ( Animator->InputData.Animation->NodeTracks[NodeTrackIndex], Ticks );
		quat Rotation = GetInterpolatedRotation ( Animator->InputData.Animation->NodeTracks[NodeTrackIndex], Ticks );
		// Calculate new transformation matrix according to the values from the animation
		math_compose_local_matrix ( &CurrentNodeTransform, Position, Rotation, Scale );
		}
	else
		CurrentNodeTransform = Animator->InputData.NodeTree[NodeIndex].DefaultTransformationMatrix;

	math_mat4_multiply ( &Animator->OutputData.NodeMatrices[NodeIndex], ParentTransform, CurrentNodeTransform );

	// Recursively apply to all sub nodes
	for ( unsigned ChildIndex = 0; ChildIndex < Animator->InputData.NodeTree[NodeIndex].ChildNodeCount; ++ChildIndex )
		{
		ApplyAnimationToNode ( Animator, Ticks, Animator->InputData.NodeTree[NodeIndex].ChildNodes[ChildIndex], Animator->OutputData.NodeMatrices[NodeIndex] );
		}
	}

/**
 * For an animation to be compatible with a node tree, there are a few conditions:
 * - at least one node in the animation must be found in the node tree
 */
bool IsAnimationCompatibleWithNodeTree ( const struct AnimationDescriptor *Animation, const struct MeshTreeNode *Nodes, const unsigned NodeCount )
	{
	unsigned MatchingNodes = 0;
	for ( unsigned TrackIndex = 0; TrackIndex < Animation->NodeTrackCount; ++TrackIndex )
		{
		for ( unsigned NodeIndex = 0; NodeIndex < NodeCount; NodeIndex++ )
			{
			if ( strcmp ( Nodes[NodeIndex].NodeName, Animation->NodeTracks[TrackIndex].NodeName ) == 0 )
				{
				++MatchingNodes;
				break;
				}
			}
		}
	return MatchingNodes > 0;
	}

bool SetupAnimator ( struct AnimatorData *Animator, const struct AnimationDescriptor *Animation, const struct MeshTreeNode *NodeTree, const unsigned NodeCount )
	{
	if ( IsAnimationCompatibleWithNodeTree ( Animation, NodeTree, NodeCount ) == false )
		{
		LOG_ERROR ( "Animation '%s' has no nodes in common with node tree", Animation->AnimationName );
		return false;
		}
	SAFE_DEL_C ( Animator->OutputData.NodeMatrices );
	SAFE_DEL_C ( Animator->OutputData.NodeToTrack );

	Animator->CurrentTime = 0.0f;
	Animator->InputData.NodeTree = NodeTree;
	Animator->InputData.NodeCount = NodeCount;

	Animator->OutputData.NodeMatrices = calloc ( Animator->InputData.NodeCount, sizeof ( mat4 ) );
	for ( unsigned Index = 0; Index < Animator->InputData.NodeCount; ++Index )
		math_mat4_set_identity_matrix ( &Animator->OutputData.NodeMatrices[Index] );

	Animator->OutputData.NodeToTrack = calloc ( Animator->InputData.NodeCount, sizeof ( int ) );
	Animator->InputData.Animation = Animation;

	for ( unsigned NodeIndex = 0; NodeIndex < Animator->InputData.NodeCount; ++NodeIndex )
		{
		Animator->OutputData.NodeToTrack[NodeIndex] = FindTrackIndexInAnimation ( Animator->InputData.Animation, Animator->InputData.NodeTree[NodeIndex].NodeName );
		}
	// TODO - Merge nodes that don't have a corresponding animation track together, so that I can save on recursion, multiplications, and memory.

	return true;
	}

void SetTime ( struct AnimatorData *Animator, const float Time )
	{
	Animator->CurrentTime = fmod ( Time, Animator->InputData.Animation->Duration );
	ApplyAnimationToNode ( Animator, fmod ( Time, Animator->InputData.Animation->Duration ), 0, MATH_MAT4_IDENTITY );
	}

void FreeAnimator ( struct AnimatorData *Animator )
	{
	SAFE_DEL_C ( Animator->OutputData.NodeMatrices );
	SAFE_DEL_C ( Animator->OutputData.NodeToTrack );
	memset ( Animator, 0, sizeof ( struct AnimatorData ) );
	}

unsigned FillBoneMatricesWithAnimator ( const struct AnimatorData *Animator, mat4 *BoneMatrices, const unsigned BoneMatrixCount, const struct MeshBone *Bones, const unsigned BoneCount, const mat4 MeshWorld )
	{
	for ( unsigned Index = 0; Index < BoneMatrixCount; ++Index )
		math_mat4_set_identity_matrix ( BoneMatrices + Index );

	mat4 MeshWorldInverse;
	if ( math_mat4_inverse ( &MeshWorldInverse, MeshWorld ) == false )
		return 0;

	unsigned MatchedBoneCount = 0;
	const unsigned Limit = BoneCount < BoneMatrixCount ? BoneCount : BoneMatrixCount;
	for ( unsigned BoneIndex = 0; BoneIndex < Limit; ++BoneIndex )
		{
		const int NodeIndex = Bones[BoneIndex].NodeIndex;
		if ( ( NodeIndex < 0 ) || ( ( unsigned ) NodeIndex >= Animator->InputData.NodeCount ) )
			continue;

		// Vertex is in mesh bind space. Offset takes it into bone space, the animated node brings it back to model space.
		mat4 BoneToModel;
		math_mat4_multiply ( &BoneToModel, Animator->OutputData.NodeMatrices[NodeIndex], Bones[BoneIndex].ModelToBoneMatrix );
		math_mat4_multiply ( BoneMatrices + BoneIndex, MeshWorldInverse, BoneToModel );
		++MatchedBoneCount;
		}
	return MatchedBoneCount;
	}