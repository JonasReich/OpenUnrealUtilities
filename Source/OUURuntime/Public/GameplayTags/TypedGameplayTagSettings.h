// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "TypedGameplayTagSettings.generated.h"

#if WITH_EDITORONLY_DATA
USTRUCT()
struct FTypedGameplayTagSettingsEntry
{
	GENERATED_BODY()
public:
	UPROPERTY(VisibleAnywhere)
	FString Comment;

	UPROPERTY(VisibleAnywhere)
	FGameplayTagContainer NativeRootTags;

	UPROPERTY(EditAnywhere)
	FGameplayTagContainer AdditionalRootTags;
};
#endif

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

	friend class FTypedGameplayTagSettingsDetails;

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

	// - UObject
	void PostInitProperties() override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --
#endif

private:
#if WITH_EDITOR
	void UpdateCopyForUIFromSettings();
	void UpdateSettingsFromCopyForUI();
#endif

	// These tags come from C++ declarations of gameplay tag structs and cannot be altered via the settings.
	// They are only here for reference.
	UPROPERTY()
	TMap<FName, FGameplayTagContainer> NativeRootTags;

	// These tags are defined in the settings and will be available in addition to the native root tags.
	UPROPERTY(Config)
	TMap<FName, FGameplayTagContainer> AdditionalRootTags;

#if WITH_EDITORONLY_DATA
	// These are not the settings stored in the ini file, but a copy that is better to read/edit in the UI.
	// Updates must be propagated to AdditionalRootTags.
	UPROPERTY(EditAnywhere, meta = (ForceInlineRow, ReadOnlyKeys, DisplayName = "Gameplay Tag Types"))
	TMap<FName, FTypedGameplayTagSettingsEntry> SettingsCopyForUI;
#endif
};
