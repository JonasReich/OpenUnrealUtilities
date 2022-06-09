// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Animation/Skeleton.h"

namespace OUU::Runtime::Animation
{
	constexpr int32 ROOT_BONE_IDX = 0;

	enum class ETraverseBoneTreeAction
	{
		// Continue traversal with child bones
		ContinueWithChildBones,
		// Continue traversal but skip child bones of current bone index
		SkipChildBones,
		// Stop traversal after the current bone
		Stop
	};

	/**
	 * Traverse through all bone indices in a skeleton root to leaf starting from a given root bone index.
	 * @param	Skeleton			The skeleton through which to iterate
	 * @param	Predicate			This functional parameter is invoked for every bone index in the tree. The result
	 *								determines how the traversal is continued.
	 * @param	StartBoneIndex		Index of the bone at which to start the traversal.
	 * @tparam	PredicateType		ETraverseBonesAction(int32)
	 */
	template <typename PredicateType>
	void TraverseBoneTree(USkeleton* Skeleton, PredicateType Predicate, int32 StartBoneIndex = ROOT_BONE_IDX)
	{
		Private::TraverseBoneTreeImpl(Skeleton, StartBoneIndex, Predicate);
	}

	namespace Private
	{
		template <typename PredicateType>
		ETraverseBoneTreeAction TraverseBoneTreeImpl(USkeleton* Skeleton, int32 BoneIndex, PredicateType Predicate)
		{
			const ETraverseBoneTreeAction NextAction = Predicate(BoneIndex);
			if (NextAction == ETraverseBoneTreeAction::ContinueWithChildBones)
			{
				TArray<int32> ChildBones;
				Skeleton->GetChildBones(BoneIndex, OUT ChildBones);
				for (int32 ChildBoneIndex : ChildBones)
				{
					const ETraverseBoneTreeAction ChildAction =
						TraverseBoneTreeImpl(Skeleton, ChildBoneIndex, Predicate);
					if (ChildAction == ETraverseBoneTreeAction::Stop)
						return ETraverseBoneTreeAction::Stop;
				}
			}
			return NextAction;
		}
	} // namespace Private
} // namespace OUU::Runtime::Animation
