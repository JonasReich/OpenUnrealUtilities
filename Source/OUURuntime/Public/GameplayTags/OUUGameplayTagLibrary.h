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
	UFUNCTION(BlueprintPure)
	static FGameplayTag GetParentTag(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure)
	static FGameplayTagContainer GetChildTags(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure)
	static int32 GetTagDepth(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure)
	static FGameplayTag GetTagUntilDepth(const FGameplayTag& Tag, int32 Depth);

	UFUNCTION(BlueprintPure)
	static TArray<FName> GetTagComponents(const FGameplayTag& Tag);

	UFUNCTION(BlueprintPure)
	static FGameplayTag CreateTagFromComponents(const TArray<FName>& TagComponents);

	UFUNCTION(BlueprintPure)
	static TArray<FGameplayTag> GetGameplayTagArrayFromContainer(const FGameplayTagContainer& Container);
};
