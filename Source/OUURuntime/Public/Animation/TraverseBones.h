// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Templates/ReverseIterator.h"

namespace OUU::Runtime::Animation
{
	constexpr int32 ROOT_BONE_IDX = 0;

	/**
	 * Utility container to support ranged-for loop based on bone-chain in reference skeleton.
	 * Usage:
	 * for (int32 BoneIndex : FBoneChainRange(ReferenceSkeleton, LeafBoneIndex)) { ... }
	 */
	struct FBoneChainRange
	{
		enum class ELeafStatus
		{
			Include,
			Exclude
		};

		FBoneChainRange(
			const FReferenceSkeleton& ReferenceSkeleton,
			int32 LeafBoneIndex,
			ELeafStatus KeepLeaf = ELeafStatus::Include)
		{
			BoneChain_RootToLeaf.Add(LeafBoneIndex);
			ReferenceSkeleton.EnsureParentsExistAndSort(BoneChain_RootToLeaf);
			if (KeepLeaf == ELeafStatus::Exclude)
			{
				BoneChain_RootToLeaf.Pop();
			}
		}

		auto begin() const noexcept { return MakeReverseIterator(IteratorUtils::end(BoneChain_RootToLeaf)); }
		auto end() const noexcept { return MakeReverseIterator(IteratorUtils::begin(BoneChain_RootToLeaf)); }

	private:
		TArray<FBoneIndexType> BoneChain_RootToLeaf;
	};

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
	 * @param Skeleton: The skeleton through which to iterate
	 * @param Predicate: This functional parameter is invoked for every bone index in the tree. The result determines
	 * how the traversal is continued.
	 * @param StartBoneIndex: Index of the bone at which to start the traversal.
	 * @tparam PredicateType: ETraverseBonesAction(int32)
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
