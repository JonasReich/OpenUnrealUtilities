// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUTextValidator.h"

#include "AssetValidation/OUUAssetValidationSettings.h"
#include "Serialization/PropertyLocalizationDataGathering.h"

bool UOUUTextValidator::CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& InContext) const
{
	// For now: do not run this validator in cook.
	// If you want validate from command-line explicitly, please run an asset validation commandlet on widget BPs
	// instead.
	if (IsRunningCookCommandlet() || IsValid(InAsset) == false)
	{
		return false;
	}

	auto AssetIsClassPredicate = [InAsset](const TSoftClassPtr<UObject>& Entry) {
		return InAsset->IsA(Entry.LoadSynchronous());
	};
	auto& Settings = UOUUAssetValidationSettings::Get();
	return Settings.ValidateNoLocalizedTextsClasses.ContainsByPredicate(AssetIsClassPredicate)
		&& (Settings.IgnoreNoLocalizedTextsClasses.ContainsByPredicate(AssetIsClassPredicate) == false);
}

EDataValidationResult UOUUTextValidator::ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& Context)
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
		for (auto& SourceSiteContext : Entry.SourceSiteContexts)
		{
			// Unreal sometimes keeps old meta data around. Also the Context.IsEditorOnly is kinda unreliable for these.
			// We just filter categories by string, because at least that works.
			if (SourceSiteContext.SiteDescription.Contains(TEXT("MetaData.Category")))
			{
				continue;
			}

			if (SourceSiteContext.IsEditorOnly == false)
				bEditorOnly = false;

			SiteDescription += TEXT(" ") + SourceSiteContext.SiteDescription;
		}

		if (bEditorOnly)
			continue;

		auto ErrorMessage = FText::Format(
			INVTEXT("Text \"{0}\" is neither a culture invariant nor linked "
					"from a string table. Source: {1}"),
			FText::FromString(Entry.SourceData.SourceString),
			FText::FromString(SiteDescription));
		Context.AddError(ErrorMessage);
		return EDataValidationResult::Invalid;
	}
	return Result;
}
