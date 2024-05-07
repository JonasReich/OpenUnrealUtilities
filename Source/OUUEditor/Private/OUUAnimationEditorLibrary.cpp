// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUAnimationEditorLibrary.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"
#include "Animation/BoneChainRange.h"
#include "Animation/TraverseBoneTree.h"
#include "Engine/SkeletalMesh.h"
#include "MeshUtilities.h"
#include "Misc/RegexUtils.h"
#include "Modules/ModuleManager.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "ScopedTransaction.h"
#include "SkeletalMeshEditorSubsystem.h"

bool UOUUAnimationEditorLibrary::ImplementsAnimationLayerInterface(
	TSubclassOf<UAnimInstance> AnimInstanceClass,
	TSubclassOf<UAnimLayerInterface> AnimationLayerInterface)
{
	if (!ensure(IsValid(AnimInstanceClass) && IsValid(AnimationLayerInterface)))
		return false;

	UAnimBlueprint* AnimationBlueprint = Cast<UAnimBlueprint>(AnimInstanceClass->ClassGeneratedBy);
	if (!ensure(IsValid(AnimationBlueprint)))
		return false;

	for (const FBPInterfaceDescription& InterfaceDesc : AnimationBlueprint->ImplementedInterfaces)
	{
		if (InterfaceDesc.Interface->IsChildOf(AnimationLayerInterface))
		{
			return true;
		}
	}

	return false;
}

USkeleton* UOUUAnimationEditorLibrary::GetAnimInstanceClassTargetSkeleton(TSubclassOf<UAnimInstance> AnimInstanceClass)
{
	if (!ensure(IsValid(AnimInstanceClass)))
		return nullptr;

	const UAnimBlueprint* AnimationBlueprint = Cast<UAnimBlueprint>(AnimInstanceClass->ClassGeneratedBy);
	if (!ensure(IsValid(AnimationBlueprint)))
		return nullptr;

	return AnimationBlueprint->TargetSkeleton;
}

USkeleton* UOUUAnimationEditorLibrary::GetAnimInstanceTargetSkeleton(const UAnimInstance* AnimInstance)
{
	if (!ensure(IsValid(AnimInstance)))
		return nullptr;

	return GetAnimInstanceClassTargetSkeleton(AnimInstance->GetClass());
}

int32 UOUUAnimationEditorLibrary::RemoveUnskinnedBonesFromMesh(
	USkeletalMesh* SkeletalMesh,
	const FString& BoneNameIncludePattern,
	const FString& BoneNameExcludePattern,
	int32 MinLOD /*= 0*/)
{
	if (!IsValid(SkeletalMesh))
		return false;

	TSet<int32> BonesToRemove_Indices;
	TSet<int32> BonesToKeep_Indices;
	const FSkeletalMeshLODRenderData& LODData = SkeletalMesh->GetResourceForRendering()->LODRenderData[MinLOD];
	auto* Skeleton = SkeletalMesh->GetSkeleton();

	enum class EFilterAction
	{
		Unknown,
		Exclude,
		Include
	};

	auto FilterBoneName = [&](int32 MeshBoneIndex, FName BoneName) -> EFilterAction {
		// Skip bone that are vertex weighted / skinned -> continue with children
		if (LODData.ActiveBoneIndices.Find(MeshBoneIndex) != INDEX_NONE)
			return EFilterAction::Exclude;
		const auto BoneNameString = BoneName.ToString();
		if (BoneNameIncludePattern.Len() > 0
			&& !OUU::Runtime::RegexUtils::MatchesRegex(BoneNameIncludePattern, BoneNameString))
			return EFilterAction::Unknown;
		if (BoneNameExcludePattern.Len() > 0
			&& OUU::Runtime::RegexUtils::MatchesRegex(BoneNameExcludePattern, BoneNameString))
			return EFilterAction::Exclude;
		return EFilterAction::Include;
	};

	using OUU::Runtime::Animation::EBoneChainLeaf;
	using OUU::Runtime::Animation::ETraverseBoneTreeAction;
	using OUU::Runtime::Animation::TBoneChainRange;

	OUU::Runtime::Animation::TraverseBoneTree(Skeleton, [&](int32 SkeletonBoneIndex) -> ETraverseBoneTreeAction {
		const auto BoneName = Skeleton->GetReferenceSkeleton().GetBoneName(SkeletonBoneIndex);
		const int32 MeshBoneIndex = SkeletalMesh->GetRefSkeleton().FindRawBoneIndex(BoneName);
		// Bone does not exist on this mesh. Children can also be skipped. No actions can be taken.
		if (MeshBoneIndex == INDEX_NONE)
			return ETraverseBoneTreeAction::SkipChildBones;

		switch (FilterBoneName(MeshBoneIndex, BoneName))
		{
		case EFilterAction::Unknown:
		{
			// This bone is neither included nor excluded. Continue checking child bones
			return ETraverseBoneTreeAction::ContinueWithChildBones;
		}
		case EFilterAction::Exclude:
		{
			BonesToKeep_Indices.Add(MeshBoneIndex);
			// This bone is excluded. If any parent bone was included we must remove it from the list.
			for (const FBoneIndexType ParentBoneIndex :
				 TBoneChainRange(SkeletalMesh->GetRefSkeleton(), MeshBoneIndex, EBoneChainLeaf::Exclude))
			{
				BonesToKeep_Indices.Add(ParentBoneIndex);
				if (BonesToRemove_Indices.Contains(ParentBoneIndex))
					BonesToRemove_Indices.Remove(ParentBoneIndex);
			}
			// This bone is excluded. Child bones could be included though, so keep checking.
			return ETraverseBoneTreeAction::ContinueWithChildBones;
		}
		case EFilterAction::Include:
		{
			// This bone is included. Add it to the list and continue checking child bones.
			if (!BonesToKeep_Indices.Contains(MeshBoneIndex))
			{
				BonesToRemove_Indices.Add(MeshBoneIndex);
			}
			return ETraverseBoneTreeAction::ContinueWithChildBones;
		}
		default:
		{
			checkNoEntry();
			return ETraverseBoneTreeAction::Stop;
		}
		}
	});

	TSet<FName> BonesToRemove_FilteredNames;
	for (const int32 MeshBoneIndex : BonesToRemove_Indices)
	{
		auto BoneName = SkeletalMesh->GetRefSkeleton().GetBoneName(MeshBoneIndex);
		bool bIsImplicitlyExcluded = false;
		for (const FBoneIndexType ParentMeshBoneIndex :
			 TBoneChainRange(SkeletalMesh->GetRefSkeleton(), MeshBoneIndex, EBoneChainLeaf::Exclude))
		{
			if (BonesToRemove_Indices.Contains(ParentMeshBoneIndex))
			{
				bIsImplicitlyExcluded = true;
				break;
			}
		}

		if (!bIsImplicitlyExcluded)
		{
			BonesToRemove_FilteredNames.Add(BoneName);
		}
	}

	if (BonesToRemove_FilteredNames.Num() == 0)
		return false;

	FScopedSkeletalMeshPostEditChange ScopedPostEditChange(SkeletalMesh);
	FScopedTransaction Transaction(TEXT(""), INVTEXT("Remove Unskinned Bones"), SkeletalMesh);
	SkeletalMesh->Modify();

	TArray<FName> BonesToRemove = BonesToRemove_FilteredNames.Array();
	IMeshUtilities& MeshUtilities = FModuleManager::Get().LoadModuleChecked<IMeshUtilities>("MeshUtilities");
	for (int32 Index = MinLOD; Index < SkeletalMesh->GetLODNum(); ++Index)
	{
		MeshUtilities.RemoveBonesFromMesh(SkeletalMesh, Index, &BonesToRemove);
		SkeletalMesh->AddBoneToReductionSetting(Index, BonesToRemove);
	}

	// Value of 0 means keep existing number of LODs
	constexpr int32 NewLodCount = 0;
	constexpr bool bRegenerateEvenIfImported = true;
	constexpr bool bGenerateBaseLOD = true;
	USkeletalMeshEditorSubsystem::RegenerateLOD(SkeletalMesh, NewLodCount, bRegenerateEvenIfImported, bGenerateBaseLOD);

	return true;
}
