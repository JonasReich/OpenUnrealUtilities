// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUGameplayTagLibrary.generated.h"

UCLASS()
class UOUUGameplayTagLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "OUU|Gameplay Tags")
	static FGameplayTag GetParentTag(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure, Category = "OUU|Gameplay Tags")
	static FGameplayTagContainer GetChildTags(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure, Category = "OUU|Gameplay Tags")
	static int32 GetTagDepth(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure, Category = "OUU|Gameplay Tags")
	static FGameplayTag GetTagUntilDepth(const FGameplayTag& Tag, int32 Depth);

	UFUNCTION(BlueprintPure, Category = "OUU|Gameplay Tags")
	static TArray<FName> GetTagComponents(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure, Category = "OUU|Gameplay Tags")
	static FGameplayTag CreateTagFromComponents(const TArray<FName>& TagComponents);
};
