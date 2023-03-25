// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

#include "TypedGameplayTagSettings.generated.h"

/**
 * Settings for typed gameplay tags.
 * In here you can add additional type info to gameplay tags.
 * Each key of the root tag maps is a "TypedGameplayTag" struct type.
 * The values are possible root tags for the respective gameplay tags & tag containers.
 */
UCLASS(DefaultConfig, Config = Game)
class OUURUNTIME_API UTypedGameplayTagSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static void GetAdditionalRootTags(FGameplayTagContainer& OutRootTags, UStruct* BlueprintStruct);
	static void AddNativeRootTags(const FGameplayTagContainer& RootTags, UStruct* BlueprintStruct);
	static void GetAllRootTags(FGameplayTagContainer& OutRootTags, UStruct* BlueprintStruct);
	// Get all leaf tags (tags without children) for the given typed gameplay tag
	static void GetAllLeafTags(FGameplayTagContainer& OutLeafTags, UStruct* BlueprintStruct);

#if WITH_EDITOR
	/**
	 * Clean up additional root tags that do not have a matching native root tag entry.
	 * This removes old config entries that do not have a matching struct anymore, e.g. after a struct was deleted or
	 * renamed.
	 */
	UFUNCTION(CallInEditor)
	static void CleanAdditionalTags();
#endif

private:
	// These tags come from C++ declarations of gameplay tag structs and cannot be altered via the settings.
	// They are only here for reference.
	UPROPERTY(VisibleAnywhere)
	TMap<FName, FGameplayTagContainer> NativeRootTags;

	// These tags are defined in the settings and will be available in addition to the native root tags.
	UPROPERTY(Config, EditAnywhere, meta = (ReadOnlyKeys))
	TMap<FName, FGameplayTagContainer> AdditionalRootTags;
};
