// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUTextValidator.h"

#include "AssetValidation/OUUAssetValidationSettings.h"
#include "Components/Widget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Serialization/PropertyLocalizationDataGathering.h"
#include "WidgetBlueprint.h"

bool UOUUTextValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	// For now: do not run this validator in cook.
	// If you want validate from command-line explicitly, please run an asset validation commandlet on widget BPs
	// instead.
	if (IsRunningCookCommandlet() || IsValid(InAsset) == false)
	{
		return false;
	}

	auto AssetIsClassPredicate = [InAsset](TSoftClassPtr<UObject> Entry) {
		return InAsset->IsA(Entry.LoadSynchronous());
	};
	auto& Settings = UOUUAssetValidationSettings::Get();
	return Settings.ValidateNoLocalizedTextsClasses.ContainsByPredicate(AssetIsClassPredicate)
		&& (Settings.IgnoreNoLocalizedTextsClasses.ContainsByPredicate(AssetIsClassPredicate) == false);
}

EDataValidationResult UOUUTextValidator::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	const auto* Package = InAsset->GetPackage();

	TArray<UObject*> Objects;
	constexpr bool bIncludeNestedObjects = true;
	GetObjectsWithPackage(Package, OUT Objects, bIncludeNestedObjects);

	TArray<FGatherableTextData> GatherableTextDataArray;
	EPropertyLocalizationGathererResultFlags ResultFlags;
	FPropertyLocalizationDataGatherer Gatherer{OUT GatherableTextDataArray, Package, OUT ResultFlags};

	for (auto& Entry : GatherableTextDataArray)
	{
		FString SiteDescription;
		bool bEditorOnly = true;
		for (auto& Context : Entry.SourceSiteContexts)
		{
			// Unreal sometimes keeps old meta data around. Also the Context.IsEditorOnly is kinda unreliable for these.
			// We just filter categories by string, because at least that works.
			if (Context.SiteDescription.Contains(TEXT("MetaData.Category")))
			{
				continue;
			}

			if (Context.IsEditorOnly == false)
				bEditorOnly = false;

			SiteDescription += TEXT(" ") + Context.SiteDescription;
		}

		if (bEditorOnly)
			continue;

		Result = EDataValidationResult::Invalid;
		AssetFails(
			InAsset,
			FText::Format(
				INVTEXT("Text \"{0}\" is neither a culture invariant nor linked "
						"from a string table. Source: {1}"),
				FText::FromString(Entry.SourceData.SourceString),
				FText::FromString(SiteDescription)),
			IN OUT ValidationErrors);
	}

	if (Result == EDataValidationResult::Valid)
	{
		AssetPasses(InAsset);
	}
	return Result;
}
