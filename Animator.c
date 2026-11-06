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
static unsigned FindNodeTrackKeyForTime ( const NodeTrackKey *Keys, const unsigned KeyCount, const double TimeStamp )
	{
	for ( unsigned KeyIndex = 0; KeyIndex < KeyCount; ++KeyIndex )
		{
		if ( Keys[KeyIndex].Time > TimeStamp )
			return KeyIndex;
		}
	return KeyCount;
	}

static bool GetInterpolatedPosition ( const NodeTrack AnimationTrack, const double TimeStamp, vec3 *Output )
	{
	if ( AnimationTrack.PositionCount == 0 )
		return false;
	if ( AnimationTrack.PositionCount == 1 )
		{
		*Output = AnimationTrack.Positions[0].Vec3Value;
		return true;
		}

	unsigned NextKeyIndex = FindNodeTrackKeyForTime ( AnimationTrack.Positions, AnimationTrack.PositionCount, TimeStamp );
	if ( NextKeyIndex == 0 )
		{
		*Output = AnimationTrack.Positions[0].Vec3Value;
		return true;
		}
	if ( NextKeyIndex >= AnimationTrack.PositionCount )
		{
		*Output = AnimationTrack.Positions[AnimationTrack.PositionCount - 1].Vec3Value;
		return true;
		}

	// Interpolate between this value and the next
	double TimeBetweenKeys = AnimationTrack.Positions[NextKeyIndex].Time - AnimationTrack.Positions[NextKeyIndex - 1].Time;
	if ( TimeBetweenKeys <= 0.0 )
		{
		*Output = AnimationTrack.Positions[NextKeyIndex - 1].Vec3Value;
		return true;
		}
	double Factor = ( TimeStamp - AnimationTrack.Positions[NextKeyIndex - 1].Time ) / TimeBetweenKeys;

	vec3 FinalPosition;
	math_vec3_lerp ( &FinalPosition, AnimationTrack.Positions[NextKeyIndex - 1].Vec3Value, AnimationTrack.Positions[NextKeyIndex].Vec3Value, Factor );
	*Output = FinalPosition;
	return true;
	}

static bool GetInterpolatedRotation ( const NodeTrack AnimationTrack, const double TimeStamp, quat *Output )
	{
	if ( AnimationTrack.RotationCount == 0 )
		return false;
	if ( AnimationTrack.RotationCount == 1 )
		{
		*Output = AnimationTrack.Rotations[0].QuatValue;
		return true;
		}

	unsigned NextKeyIndex = FindNodeTrackKeyForTime ( AnimationTrack.Rotations, AnimationTrack.RotationCount, TimeStamp );
	if ( NextKeyIndex == 0 )
		{
		*Output = AnimationTrack.Rotations[0].QuatValue;
		return true;
		}
	if ( NextKeyIndex >= AnimationTrack.RotationCount )
		{
		*Output = AnimationTrack.Rotations[AnimationTrack.RotationCount - 1].QuatValue;
		return true;
		}

	// Interpolate between this value and the next
	double TimeBetweenKeys = AnimationTrack.Rotations[NextKeyIndex].Time - AnimationTrack.Rotations[NextKeyIndex - 1].Time;
	if ( TimeBetweenKeys <= 0.0 )
		{
		*Output = AnimationTrack.Rotations[NextKeyIndex - 1].QuatValue;
		return true;
		}
	double Factor = ( TimeStamp - AnimationTrack.Rotations[NextKeyIndex - 1].Time ) / TimeBetweenKeys;

	quat FinalRotation;
	math_quat_slerp ( &FinalRotation, AnimationTrack.Rotations[NextKeyIndex - 1].QuatValue, AnimationTrack.Rotations[NextKeyIndex].QuatValue, Factor );
	*Output = FinalRotation;
	return true;
	}

static bool GetInterpolatedScale ( const NodeTrack AnimationTrack, const double TimeStamp, vec3 *Output )
	{
	if ( AnimationTrack.ScaleCount == 0 )
		return false;
	if ( AnimationTrack.ScaleCount == 1 )
		{
		*Output = AnimationTrack.Scales[0].Vec3Value;
		return true;
		}

	unsigned NextKeyIndex = FindNodeTrackKeyForTime ( AnimationTrack.Scales, AnimationTrack.ScaleCount, TimeStamp );
	if ( NextKeyIndex == 0 )
		{
		*Output = AnimationTrack.Scales[0].Vec3Value;
		return true;
		}
	if ( NextKeyIndex >= AnimationTrack.ScaleCount )
		{
		*Output = AnimationTrack.Scales[AnimationTrack.ScaleCount - 1].Vec3Value;
		return true;
		}

	// Interpolate between this value and the next
	double TimeBetweenKeys = AnimationTrack.Scales[NextKeyIndex].Time - AnimationTrack.Scales[NextKeyIndex - 1].Time;
	if ( TimeBetweenKeys <= 0.0 )
		{
		*Output = AnimationTrack.Scales[NextKeyIndex - 1].Vec3Value;
		return true;
		}
	double Factor = ( TimeStamp - AnimationTrack.Scales[NextKeyIndex - 1].Time ) / TimeBetweenKeys;

	vec3 FinalScale;
	math_vec3_lerp ( &FinalScale, AnimationTrack.Scales[NextKeyIndex - 1].Vec3Value, AnimationTrack.Scales[NextKeyIndex].Vec3Value, Factor );
	*Output = FinalScale;
	return true;
	}

static void ApplyAnimationToNode ( AnimatorData *Animator, const double Ticks, const unsigned NodeIndex, const mat4 ParentTransform )
	{
	mat4 CurrentNodeTransform;

	// See if this node has a track in this animation. If so, animate it
	int NodeTrackIndex = Animator->OutputData.NodeToTrack[NodeIndex];
	if ( NodeTrackIndex != -1 )
		{
		const NodeTrack *Track = &Animator->InputData.Animation->NodeTracks[NodeTrackIndex];
		vec3 Position;
		vec3 Scale;
		quat Rotation;
		math_decompose_local_matrix ( Animator->InputData.NodeTree[NodeIndex].DefaultTransformationMatrix, &Position, &Rotation, &Scale );
		GetInterpolatedPosition ( *Track, Ticks, &Position );
		GetInterpolatedRotation ( *Track, Ticks, &Rotation );
		GetInterpolatedScale ( *Track, Ticks, &Scale );
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

bool SetupAnimator ( AnimatorData *Animator, const struct AnimationDescriptor *Animation, const struct MeshTreeNode *NodeTree, const unsigned NodeCount )
	{
	if ( IsAnimationCompatibleWithNodeTree ( Animation, NodeTree, NodeCount ) == false )
		{
		LOG_ERROR ( "Animation '%s' has no nodes in common with node tree", Animation->AnimationName );
		return false;
		}
	SAFE_DEL_C ( Animator->OutputData.NodeMatrices );
	SAFE_DEL_C ( Animator->OutputData.NodeToTrack );

	Animator->CurrentTime = 0.0;
	Animator->InputData.NodeTree = NodeTree;
	Animator->InputData.NodeCount = NodeCount;

	Animator->OutputData.NodeMatrices = calloc ( Animator->InputData.NodeCount, sizeof ( mat4 ) );
	if ( Animator->OutputData.NodeMatrices == NULL )
		{
		LOG_ERROR ( "Failed to allocate memory for node matrices" );
		FreeAnimator ( Animator );
		return false;
		}
	for ( unsigned Index = 0; Index < Animator->InputData.NodeCount; ++Index )
		math_mat4_set_identity_matrix ( &Animator->OutputData.NodeMatrices[Index] );

	Animator->OutputData.NodeToTrack = calloc ( Animator->InputData.NodeCount, sizeof ( int ) );
	if ( Animator->OutputData.NodeToTrack == NULL )
		{
		LOG_ERROR ( "Failed to allocate memory for node to track mapping" );
		FreeAnimator ( Animator );
		return false;
		}
	Animator->InputData.Animation = Animation;

	for ( unsigned NodeIndex = 0; NodeIndex < Animator->InputData.NodeCount; ++NodeIndex )
		{
		Animator->OutputData.NodeToTrack[NodeIndex] = FindTrackIndexInAnimation ( Animator->InputData.Animation, Animator->InputData.NodeTree[NodeIndex].NodeName );
		}
	// TODO - Merge nodes that don't have a corresponding animation track nor meshes together, so that I can save on recursion, multiplications, and memory.

	return true;
	}

void SetTime ( AnimatorData *Animator, const double Time )
	{
	if ( Animator->InputData.Animation == NULL )
		return;

	const double Duration = Animator->InputData.Animation->Duration;
	double Wrapped = 0.0;
	if ( Duration > 0.0 )
		{
		Wrapped = fmod ( Time, Duration );
		if ( Wrapped < 0.0 )
			Wrapped += Duration;
		}
	Animator->CurrentTime = Wrapped;
	ApplyAnimationToNode ( Animator, Wrapped, 0, MATH_MAT4_IDENTITY );
	}

void FreeAnimator ( AnimatorData *Animator )
	{
	SAFE_DEL_C ( Animator->OutputData.NodeMatrices );
	SAFE_DEL_C ( Animator->OutputData.NodeToTrack );
	memset ( Animator, 0, sizeof ( AnimatorData ) );
	}

unsigned FillBoneMatricesWithAnimator ( const AnimatorData const *Animator, mat4 *BoneMatrices, const unsigned BoneMatrixCount, const struct MeshBone const *Bones, const unsigned BoneCount, const mat4 MeshWorldInverse )
	{
	unsigned MatchedBoneCount = 0;
	const unsigned MaxBone = MIN ( BoneCount, BoneMatrixCount );
	for ( unsigned BoneIndex = 0; BoneIndex < MaxBone; ++BoneIndex )
		{
		const int NodeIndex = Bones[BoneIndex].NodeIndex;
		if ( ( NodeIndex < 0 ) || ( ( unsigned ) NodeIndex >= Animator->InputData.NodeCount ) )
			{
			math_mat4_set_identity_matrix ( &BoneMatrices[BoneIndex] );
			continue;
			}
		/* Vertex * (meshWorld^-1 * boneWorld * inverseBind) stays in mesh space, which is what u_ModelMatrix expects. */
		mat4 PosedBone;
		math_mat4_multiply ( &PosedBone, Animator->OutputData.NodeMatrices[NodeIndex], Bones[BoneIndex].ModelToBoneMatrix );
		math_mat4_multiply ( &BoneMatrices[BoneIndex], MeshWorldInverse, PosedBone );
		++MatchedBoneCount;
		}
	return MatchedBoneCount;
	}