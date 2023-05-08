// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Animation/Skeleton.h"
#include "Templates/ReverseIterator.h"

namespace OUU::Runtime::Animation
{
	enum class EBoneChainDirection
	{
		RootToLeaf,
		LeafToRoot
	};

	enum class EBoneChainLeaf
	{
		Include,
		Exclude
	};

	/**
	 * Utility container to support ranged-for loop based on bone-chain in reference skeleton.
	 * Usage:
	 * for (int32 BoneIndex : FBoneChainRange(ReferenceSkeleton, LeafBoneIndex)) { ... }
	 */
	template <EBoneChainDirection Direction = EBoneChainDirection::RootToLeaf>
	struct TBoneChainRange
	{
		TBoneChainRange(
			const FReferenceSkeleton& ReferenceSkeleton,
			int32 LeafBoneIndex,
			EBoneChainLeaf LeafStatus = EBoneChainLeaf::Include)
		{
			BoneChain_RootToLeaf.Add(LeafBoneIndex);
			ReferenceSkeleton.EnsureParentsExistAndSort(BoneChain_RootToLeaf);
			if (LeafStatus == EBoneChainLeaf::Exclude)
			{
				BoneChain_RootToLeaf.Pop();
			}
		}

		auto begin() const noexcept
		{
			return MakeReverseIteratorIf<Direction == EBoneChainDirection::LeafToRoot>(
				OUU::Runtime::Private::IteratorUtils::end(BoneChain_RootToLeaf));
		}
		auto end() const noexcept
		{
			return MakeReverseIteratorIf<Direction == EBoneChainDirection::LeafToRoot>(
				OUU::Runtime::Private::IteratorUtils::begin(BoneChain_RootToLeaf));
		}

	private:
		TArray<FBoneIndexType> BoneChain_RootToLeaf;
	};
} // namespace OUU::Runtime::Animation
