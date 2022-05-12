// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimLayerInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUAnimationEditorLibrary.generated.h"

UCLASS()
class OUUEDITOR_API UOUUAnimationEditorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Animation")
	static bool ImplementsAnimationLayerInterface(
		TSubclassOf<UAnimInstance> AnimInstanceClass,
		TSubclassOf<UAnimLayerInterface> AnimationLayerInterfaceClass);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Animation")
	static USkeleton* GetAnimInstanceClassTargetSkeleton(TSubclassOf<UAnimInstance> AnimInstanceClass);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Animation")
	static USkeleton* GetAnimInstanceTargetSkeleton(UAnimInstance* AnimInstance);

	/**
	 * Remove bones without any skin weights from a skeletal mesh.
	 * Only removes the bones matching to the supplied filter rules.
	 *
	 * @param SkeletalMesh: Target skeletal mesh
	 * @param MinLOD: Minimum LOD level to remove bones from. All LOD levels below will also have the bone removed.
	 * @param BoneNameIncludePattern: If not empty, only bones matching this regex pattern will be removed.
	 * @param BoneNameExcludePattern: If not empty, bones matching this regex pattern will not be removed.
	 * If a child bone matches this pattern, the parent bone is also skipped -> Exclude pattern has higher precedence
	 * than IncludePattern.
	 * @returns if any new entries were added to the "BonesToRemove" list
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Animation")
	static int32 RemoveUnskinnedBonesFromMesh(
		USkeletalMesh* SkeletalMesh,
		const FString& BoneNameIncludePattern,
		const FString& BoneNameExcludePattern,
		int32 MinLOD = 0);
};
