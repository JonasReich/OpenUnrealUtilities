// Copyright (c) 2022 Jonas Reich

#include "Animation/OUUAnimationLibrary.h"

#include "Animation/AnimBlueprint.h"
#include "Animation/AnimInstance.h"

#if WITH_EDITOR
bool UOUUAnimationLibrary::ImplementsAnimationLayerInterface(
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

USkeleton* UOUUAnimationLibrary::GetAnimInstanceClassTargetSkeleton(TSubclassOf<UAnimInstance> AnimInstanceClass)
{
	if (!ensure(IsValid(AnimInstanceClass)))
		return nullptr;

	UAnimBlueprint* AnimationBlueprint = Cast<UAnimBlueprint>(AnimInstanceClass->ClassGeneratedBy);
	if (!ensure(IsValid(AnimationBlueprint)))
		return nullptr;

	return AnimationBlueprint->TargetSkeleton;
}

USkeleton* UOUUAnimationLibrary::GetAnimInstanceTargetSkeleton(UAnimInstance* AnimInstance)
{
	if (!ensure(IsValid(AnimInstance)))
		return nullptr;

	return GetAnimInstanceClassTargetSkeleton(AnimInstance->GetClass());
}
#endif WITH_EDITOR
