// Copyright (c) 2022 Jonas Reich

#include "GameplayTags/TypedGameplayTagSettings.h"

void UTypedGameplayTagSettings::GetAdditionalRootTags(FGameplayTagContainer& OutRootTags, UStruct* BlueprintStruct)
{
	auto& AdditionalRootTags = GetDefault<UTypedGameplayTagSettings>()->AdditionalRootTags;
	if (auto* Tags = AdditionalRootTags.Find(*BlueprintStruct->GetName()))
	{
		OutRootTags.AppendTags(*Tags);
	}
}

void UTypedGameplayTagSettings::AddNativeRootTags(const FGameplayTagContainer& RootTags, UStruct* BlueprintStruct)
{
	auto* Settings = GetMutableDefault<UTypedGameplayTagSettings>();
	FName StructName = *BlueprintStruct->GetName();
	Settings->NativeRootTags.Add(StructName, FGameplayTagContainer(RootTags));

	// also add an entry for additional tags if not already present
	Settings->AdditionalRootTags.FindOrAdd(StructName, FGameplayTagContainer::EmptyContainer);
}

void UTypedGameplayTagSettings::GetAllTags(FGameplayTagContainer& OutRootTags, UStruct* BlueprintStruct)
{
	auto& NativeRootTags = GetDefault<UTypedGameplayTagSettings>()->NativeRootTags;
	if (auto* Tags = NativeRootTags.Find(*BlueprintStruct->GetName()))
	{
		OutRootTags.AppendTags(*Tags);
	}
	GetAdditionalRootTags(OutRootTags, BlueprintStruct);
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
}
#endif
