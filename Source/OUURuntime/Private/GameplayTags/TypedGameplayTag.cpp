// Copyright (c) 2022 Jonas Reich

#include "GameplayTags/TypedGameplayTag.h"

#if WITH_EDITOR
	#include "Modules/ModuleManager.h"
	#include "GameplayTagsEditorModule.h"
	#include "PropertyEditorModule.h"
	#include "GameplayTagsManager.h"
	#include "Misc/RegexUtils.h"

namespace OUU::Runtime::Private
{
	FPropertyEditorModule& GetPropertyEditorModule()
	{
		return FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	}

	template <typename PredicateT>
	auto ForEachTypedGameplayTagType(PredicateT Predicate)
	{
		UStruct* ParentStruct = FTypedGameplayTag_Base::StaticStruct();
		for (auto* Struct : TObjectRange<UScriptStruct>())
		{
			// Exclude the parent struct itself from the results.
			if (Struct == ParentStruct)
				continue;

			if (Struct->IsChildOf(ParentStruct))
			{
				Predicate(*Struct->GetName());
			}
		}
	}

	FString MakeFilterString(const FGameplayTagContainer& GameplayTags)
	{
		FString Result;
		int i = 0;
		for (auto& Tag : GameplayTags)
		{
			Result += Tag.ToString();
			if (i < GameplayTags.Num() - 1)
			{
				Result += TEXT(",");
			}
			++i;
		}
		return Result;
	}
} // namespace OUU::Runtime::Private

#endif
void FTypedGameplayTag_Base::RegisterAllDerivedPropertyTypeLayouts()
{
	auto& TagsManager = UGameplayTagsManager::Get();
	TagsManager.OnGetCategoriesMetaFromPropertyHandle.AddLambda([](TSharedPtr<IPropertyHandle> PropertyHandle,
																   FString& OutFilterString) {
		auto* Property = PropertyHandle->GetProperty();

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			// only generate filter string for typed gameplay tags
			auto* Struct = StructProperty->Struct;
			if (Struct->IsChildOf(FTypedGameplayTag_Base::StaticStruct()))
			{
				FGameplayTagContainer AllRootTags;
				UTypedGameplayTagSettings::GetAllTags(OUT AllRootTags, Struct);
				OutFilterString = OUU::Runtime::Private::MakeFilterString(AllRootTags);
				return;
			}
		}

		auto CategoriesString = UGameplayTagsManager::Get().StaticGetCategoriesMetaFromPropertyHandle(PropertyHandle);
		auto Matches = OUU::Runtime::RegexUtils::GetRegexMatchesAndGroups("TypedTag\\{(.*)\\}", 1, CategoriesString);

		if (Matches.Num() > 1)
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Warning,
				TEXT("More than one TypedTag found in Categories metadata (%s) for property %s"),
				*CategoriesString,
				*Property->GetPathName());
		}

		if (Matches.Num() > 0)
		{
			auto TypedTagStructName = Matches[0].CaptureGroups[1].MatchString;

			for (auto* Struct : TObjectRange<UScriptStruct>())
			{
				if (Struct->IsChildOf(FTypedGameplayTag_Base::StaticStruct())
					&& Struct->GetName() == TypedTagStructName)
				{
					FGameplayTagContainer AllRootTags;
					UTypedGameplayTagSettings::GetAllTags(OUT AllRootTags, Struct);
					OutFilterString = OUU::Runtime::Private::MakeFilterString(AllRootTags);
					return;
				}
			}

			UE_LOG(
				LogOpenUnrealUtilities,
				Error,
				TEXT("Invalid struct name %s in Categories metadata (%s) for property %s"),
				*TypedTagStructName,
				*CategoriesString,
				*Property->GetPathName());
		}
	});

	FPropertyEditorModule& PropertyEditorModule = OUU::Runtime::Private::GetPropertyEditorModule();
	OUU::Runtime::Private::ForEachTypedGameplayTagType([&](FName TypeName) {
		PropertyEditorModule.RegisterCustomPropertyTypeLayout(
			TypeName,
			FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FGameplayTagCustomizationPublic::MakeInstance));
	});
}

void FTypedGameplayTag_Base::UnregisterAllDerivedPropertyTypeLayouts()
{
	FPropertyEditorModule& PropertyEditorModule = OUU::Runtime::Private::GetPropertyEditorModule();
	OUU::Runtime::Private::ForEachTypedGameplayTagType(
		[&](FName TypeName) { PropertyEditorModule.UnregisterCustomPropertyTypeLayout(TypeName); });
}

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
