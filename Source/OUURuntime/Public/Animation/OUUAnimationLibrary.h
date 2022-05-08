// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimLayerInterface.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUAnimationLibrary.generated.h"

UCLASS()
class OUURUNTIME_API UOUUAnimationLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Animation")
	static bool ImplementsAnimationLayerInterface(
		TSubclassOf<UAnimInstance> AnimInstanceClass,
		TSubclassOf<UAnimLayerInterface> AnimationLayerInterfaceClass);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Animation")
	static USkeleton* GetAnimInstanceClassTargetSkeleton(TSubclassOf<UAnimInstance> AnimInstanceClass);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Animation")
	static USkeleton* GetAnimInstanceTargetSkeleton(UAnimInstance* AnimInstance);
#endif
};
