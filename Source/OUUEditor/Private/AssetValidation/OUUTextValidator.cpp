// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUTextValidator.h"

#include "AssetValidation/OUUAssetValidationSettings.h"
#include "Components/Widget.h"
#include "WidgetBlueprint.h"

bool UOUUTextValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	// For now: do not run this validator in cook.
	// If you want validate from command-line explicitly, please run an asset validation commandlet on widget BPs
	// instead.
	if (IsRunningCookCommandlet())
	{
		return false;
	}

	return IsValid(InAsset)
		&& UOUUAssetValidationSettings::Get().ValidateNoLocalizedTextsClasses.Contains(InAsset->GetClass());
}

EDataValidationResult UOUUTextValidator::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	UClass* Class = InAsset->GetClass();
	UObject* Asset = InAsset;
	if (auto* Blueprint = Cast<UBlueprint>(InAsset))
	{
		Class = Blueprint->GeneratedClass;
		Asset = Blueprint->GeneratedClass->GetDefaultObject();

		if (auto* WidgetBlueprint = Cast<UWidgetBlueprint>(InAsset))
		{
			for (UWidget* Widget : WidgetBlueprint->GetAllSourceWidgets())
			{
				Result = CombineDataValidationResults(Result, ValidateLoadedAsset(Widget, IN OUT ValidationErrors));
			}
		}
	}
	for (auto& PropPair : TPropertyValueRange<FTextProperty>(Class, Asset))
	{
		if (PropPair.Key->HasAnyPropertyFlags(CPF_Edit) == false)
		{
			// Skip properties that are not editable.
			continue;
		}
		const FText& Text = PropPair.Key->GetPropertyValue(PropPair.Value);
		if (Text.IsCultureInvariant() || Text.IsFromStringTable() || Text.IsEmpty())
			continue;

		Result = EDataValidationResult::Invalid;
		AssetFails(
			InAsset,
			FText::Format(
				INVTEXT("Text \"{0}\" from property {1} in {2} is neither a culture invariant nor linked from a string "
						"table"),
				Text,
				FText::FromName(PropPair.Key->GetFName()),
				FText::FromName(InAsset->GetFName())),
			IN OUT ValidationErrors);
	}
	if (Result != EDataValidationResult::Valid)
	{
		AssetPasses(InAsset);
	}
	return Result;
}
