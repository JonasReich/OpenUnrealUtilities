// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/TypedGameplayTagSettings.h"

#include "GameplayTagsManager.h"

void UTypedGameplayTagSettings::GetAdditionalRootTags(FGameplayTagContainer& OutRootTags, UStruct* BlueprintStruct)
{
	const auto* Settings = GetDefault<UTypedGameplayTagSettings>();
	check(Settings);
	auto& AdditionalRootTags = Settings->AdditionalRootTags;
	if (auto* Tags = AdditionalRootTags.Find(*BlueprintStruct->GetName()))
	{
		OutRootTags.AppendTags(*Tags);
	}
}

void UTypedGameplayTagSettings::AddNativeRootTags(const FGameplayTagContainer& RootTags, UStruct* BlueprintStruct)
{
	auto* Settings = GetMutableDefault<UTypedGameplayTagSettings>();
	check(Settings);
	FName StructName = *BlueprintStruct->GetName();
	Settings->NativeRootTags.Add(StructName, FGameplayTagContainer(RootTags));

	// also add an entry for additional tags if not already present
	Settings->AdditionalRootTags.FindOrAdd(StructName, FGameplayTagContainer::EmptyContainer);

	Settings->UpdateCopyForUIFromSettings();
}

void UTypedGameplayTagSettings::GetAllRootTags(FGameplayTagContainer& OutRootTags, UStruct* BlueprintStruct)
{
	const auto* Settings = GetDefault<UTypedGameplayTagSettings>();
	check(Settings);
	auto& NativeRootTags = Settings->NativeRootTags;
	if (auto* Tags = NativeRootTags.Find(*BlueprintStruct->GetName()))
	{
		OutRootTags.AppendTags(*Tags);
	}
	GetAdditionalRootTags(OutRootTags, BlueprintStruct);
}

void UTypedGameplayTagSettings::GetAllLeafTags(FGameplayTagContainer& OutLeafTags, UStruct* BlueprintStruct)
{
	const auto& GameplayTagManager = UGameplayTagsManager::Get();

	FGameplayTagContainer RootTags;
	GetAllRootTags(RootTags, BlueprintStruct);

	FGameplayTagContainer AllTags;
	for (const auto& RootTag : RootTags)
	{
		// Root tags can also be leaf tags if they don't have any child tags.
		AllTags.AddTag(RootTag);
		AllTags.AppendTags(GameplayTagManager.RequestGameplayTagChildren(RootTag));
	}

	for (const auto& Tag : AllTags)
	{
		const auto Node = GameplayTagManager.FindTagNode(Tag);
		if (Node && Node->GetChildTagNodes().IsEmpty())
		{
			OutLeafTags.AddTag(Tag);
		}
	}
}

#if WITH_EDITOR
void UTypedGameplayTagSettings::CleanAdditionalTags()
{
	auto& Settings = *GetMutableDefault<UTypedGameplayTagSettings>();

	TArray<FName> NativeKeys;
	Settings.NativeRootTags.GetKeys(OUT NativeKeys);
	TArray<FName> AdditionalKeys;
	Settings.AdditionalRootTags.GetKeys(OUT AdditionalKeys);

	// Filter out additional keys that are not part of the native keys...
	for (auto& Key : NativeKeys)
	{
		AdditionalKeys.Remove(Key);
	}

	// ... and remove them
	for (auto& Key : AdditionalKeys)
	{
		Settings.AdditionalRootTags.Remove(Key);
	}

	Settings.UpdateCopyForUIFromSettings();
}

void UTypedGameplayTagSettings::PostInitProperties()
{
	Super::PostInitProperties();
	UpdateCopyForUIFromSettings();
}

void UTypedGameplayTagSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	UpdateSettingsFromCopyForUI();
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void UTypedGameplayTagSettings::UpdateCopyForUIFromSettings()
{
	SettingsCopyForUI.Reset();
	for (auto& Entry : NativeRootTags)
	{
		SettingsCopyForUI.FindOrAdd(Entry.Key).NativeRootTags = Entry.Value;
	}
	for (auto& Entry : AdditionalRootTags)
	{
		SettingsCopyForUI.FindOrAdd(Entry.Key).AdditionalRootTags = Entry.Value;
	}
	if (bCanLoadTooltipsFromReflectionData)
	{
		UpdateTooltips();
	}
	else
	{
		FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddUObject(this, &UTypedGameplayTagSettings::UpdateTooltips);
	}
}

void UTypedGameplayTagSettings::UpdateTooltips()
{
	bCanLoadTooltipsFromReflectionData = true;
	for (auto& Entry : SettingsCopyForUI)
	{
		if (UStruct* Struct = UClass::TryFindTypeSlow<UStruct>(*Entry.Key.ToString()))
		{
			Entry.Value.Comment = Struct->GetMetaData(TEXT("Tooltip"));
		}
		else
		{
			Entry.Value.Comment = TEXT("Failed to find structure");
		}
	}
}

void UTypedGameplayTagSettings::UpdateSettingsFromCopyForUI()
{
	// Native tags can't be changed in editor, so only update AdditionalRootTags
	AdditionalRootTags.Reset();
	for (auto& Entry : SettingsCopyForUI)
	{
		AdditionalRootTags.FindOrAdd(Entry.Key) = Entry.Value.AdditionalRootTags;
	}
}

#endif
