// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "TypedGameplayTagSettings.generated.h"

USTRUCT()
struct FTypedGameplayTagSettingsEntry
{
	GENERATED_BODY()
public:
#if WITH_EDITORONLY_DATA
	// Comment from C++ code on what this type is used for.
	UPROPERTY(VisibleAnywhere,Category="OUU|Typed Gameplay Tag|Settings")
	FString Comment;

	// Gameplay tags declared in C++ code that are always available for this FTypedGameplayTag type.
	// Can't be edited. Only here for reference.y
	UPROPERTY(VisibleAnywhere,Category="OUU|Typed Gameplay Tag|Settings")
	FGameplayTagContainer NativeRootTags;

	// Additional tags that are valid root tags for this gameplay tag type.
	// These can be either content tags (tags from INI files) or native tags from other systems.
	// This allows tag sharing between systems while still keeping separate FTypedGameplayTag types.
	UPROPERTY(EditAnywhere,Category="OUU|Typed Gameplay Tag|Settings")
	FGameplayTagContainer AdditionalRootTags;
#endif
};

/**
 * Settings for typed gameplay tags.
 * In here you can add additional type info to each FTypedGameplayTag type.
 */
UCLASS(DefaultConfig, Config = GameplayTags)
class OUURUNTIME_API UTypedGameplayTagSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static void GetAdditionalRootTags(FGameplayTagContainer& OutRootTags, const UStruct* BlueprintStruct);
	static void GetAdditionalRootTags(FGameplayTagContainer& OutRootTags, const FName& BlueprintStructName);
	template <typename CallableT>
	static bool ForEachAdditionalRootTag(const CallableT& Callable, const UStruct* BlueprintStruct);
	template <typename CallableT>
	static bool ForEachAdditionalRootTag(const CallableT& Callable, const FName& BlueprintStructName);
	static void AddNativeRootTags(const FGameplayTagContainer& RootTags, const UStruct* BlueprintStruct);
	static void GetAllRootTags(FGameplayTagContainer& OutRootTags, const UStruct* BlueprintStruct);
	static void GetAllRootTags(FGameplayTagContainer& OutRootTags, const FName& BlueprintStructName);
	// Get all leaf tags (tags without children) for the given typed gameplay tag
	static void GetAllLeafTags(FGameplayTagContainer& OutLeafTags, const UStruct* BlueprintStruct);

#if WITH_EDITOR
	/**
	 * Clean up additional root tags that do not have a matching native root tag entry.
	 * This removes old config entries that do not have a matching struct anymore, e.g. after a struct was deleted or
	 * renamed.
	 */
	UFUNCTION(CallInEditor,Category="OUU|Typed Gameplay Tag|Settings")
	static void CleanAdditionalTags();

	// - UObject
	void PostInitProperties() override;
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	// --

	// - UDeveloperSettings
	FName GetCategoryName() const override { return TEXT("Project"); }
	FText GetSectionText() const override { return INVTEXT("Gameplay Tags (OUU Typed Tags)"); }
	// --
#endif

private:
#if WITH_EDITOR
	void UpdateCopyForUIFromSettings();
	UFUNCTION()
	void UpdateTooltips();
	void UpdateSettingsFromCopyForUI();
#endif

	bool bCanLoadTooltipsFromReflectionData = false;

	UPROPERTY()
	TMap<FName, FGameplayTagContainer> NativeRootTags;

	UPROPERTY(Config)
	TMap<FName, FGameplayTagContainer> AdditionalRootTags;

#if WITH_EDITORONLY_DATA
	// These are not the settings stored in the ini file, but a copy that is better to read/edit in the UI.
	// Updates are automatically propagated to AdditionalRootTags, which is saved to the INI file.
	UPROPERTY(EditAnywhere, meta = (ForceInlineRow, ReadOnlyKeys, DisplayName = "Gameplay Tag Types"),Category="OUU|Typed Gameplay Tag|Settings")
	TMap<FName, FTypedGameplayTagSettingsEntry> SettingsCopyForUI;
#endif
};

template <typename CallableT>
bool UTypedGameplayTagSettings::ForEachAdditionalRootTag(const CallableT& Callable, const UStruct* BlueprintStruct)
{
	return ForEachAdditionalRootTag(Callable, BlueprintStruct->GetFName());
}

template <typename CallableT>
bool UTypedGameplayTagSettings::ForEachAdditionalRootTag(const CallableT& Callable, const FName& BlueprintStructName)
{
	const auto* Settings = GetDefault<UTypedGameplayTagSettings>();
	check(Settings);
	auto& AdditionalRootTags = Settings->AdditionalRootTags;
	if (auto* Tags = AdditionalRootTags.Find(BlueprintStructName))
	{
		for (const auto& Tag : *Tags)
		{
			if (Callable(Tag))
			{
				return true;
			}
		}
	}

	return false;
}