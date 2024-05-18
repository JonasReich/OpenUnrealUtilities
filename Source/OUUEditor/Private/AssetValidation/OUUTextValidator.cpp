// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUTextValidator.h"

#include "AssetValidation/OUUAssetValidationSettings.h"
#include "Components/Widget.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Serialization/PropertyLocalizationDataGathering.h"
#include "WidgetBlueprint.h"

void GatherBlueprintForLocalization(const UBlueprint* const Blueprint)
{
	// Force non-data-only blueprints to set the HasScript flag, as they may not currently have bytecode due to a
	// compilation error
	bool bForceHasScript = !FBlueprintEditorUtils::IsDataOnlyBlueprint(Blueprint);
	if (bForceHasScript)
		return;

	// Also do this for blueprints that derive from something containing text properties, as these may propagate default
	// values from their parent class on load
	if (UClass* BlueprintParentClass = Blueprint->ParentClass.Get())
	{
		TArray<UStruct*> TypesToCheck;
		TypesToCheck.Add(BlueprintParentClass);

		TSet<UStruct*> TypesChecked;
		while (!bForceHasScript && TypesToCheck.Num() > 0)
		{
			UStruct* TypeToCheck = TypesToCheck.Pop(/*bAllowShrinking*/ false);
			TypesChecked.Add(TypeToCheck);

			for (TFieldIterator<const FProperty> PropIt(
					 TypeToCheck,
					 EFieldIteratorFlags::IncludeSuper,
					 EFieldIteratorFlags::ExcludeDeprecated,
					 EFieldIteratorFlags::IncludeInterfaces);
				 !bForceHasScript && PropIt;
				 ++PropIt)
			{
				auto ProcessInnerProperty =
					[&bForceHasScript, &TypesToCheck, &TypesChecked](const FProperty* InProp) -> bool {
					if (const FTextProperty* TextProp = CastField<const FTextProperty>(InProp))
					{
						bForceHasScript = true;
						return true;
					}
					if (const FStructProperty* StructProp = CastField<const FStructProperty>(InProp))
					{
						if (!TypesChecked.Contains(StructProp->Struct))
						{
							TypesToCheck.Add(StructProp->Struct);
						}
						return true;
					}
					return false;
				};

				if (!ProcessInnerProperty(*PropIt))
				{
					if (const FArrayProperty* ArrayProp = CastField<const FArrayProperty>(*PropIt))
					{
						ProcessInnerProperty(ArrayProp->Inner);
					}
					if (const FMapProperty* MapProp = CastField<const FMapProperty>(*PropIt))
					{
						ProcessInnerProperty(MapProp->KeyProp);
						ProcessInnerProperty(MapProp->ValueProp);
					}
					if (const FSetProperty* SetProp = CastField<const FSetProperty>(*PropIt))
					{
						ProcessInnerProperty(SetProp->ElementProp);
					}
				}
			}
		}
	}
}

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
		&& UOUUAssetValidationSettings::Get().ValidateNoLocalizedTextsClasses.ContainsByPredicate(
			[InAsset](TSoftClassPtr<UObject> Entry) { return InAsset->IsA(Entry.LoadSynchronous()); });
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

	auto OwnerStructsToIgnore =
		TArray<UStruct*>{FBPVariableDescription::StaticStruct(), FBPEditorBookmarkNode::StaticStruct()};

	TArray<FGatherableTextData> GatherableTextDataArray;
	EPropertyLocalizationGathererResultFlags ResultFlags;
	FPropertyLocalizationDataGatherer Gatherer{OUT GatherableTextDataArray, Package, OUT ResultFlags};

	for (auto& Entry : GatherableTextDataArray)
	{
		FString SiteDescription;
		bool bEditorOnly = true;
		for (auto& Context : Entry.SourceSiteContexts)
		{
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
