// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/TypedGameplayTag.h"

#include "GameplayTagsManager.h"
#include "Misc/RegexUtils.h"
#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"

#if WITH_EDITOR
	#include "GameplayTagsEditorModule.h"
	#include "PropertyEditorModule.h"
	#include "PropertyHandle.h"
#endif

#if WITH_EDITOR
namespace OUU::Runtime::Private
{
	FPropertyEditorModule& GetPropertyEditorModule()
	{
		return FModuleManager::LoadModuleChecked<FPropertyEditorModule>(TEXT("PropertyEditor"));
	}

	template <typename PredicateT>
	auto ForEachTypedGameplayTagType(PredicateT Predicate)
	{
		const UStruct* ParentStruct = FTypedGameplayTag_Base::StaticStruct();
		for (const auto* Struct : TObjectRange<UScriptStruct>())
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

void FTypedGameplayTag_Base::RegisterAllDerivedPropertyTypeLayouts()
{
	auto& TagsManager = UGameplayTagsManager::Get();
	TagsManager.OnGetCategoriesMetaFromPropertyHandle.AddLambda([](const TSharedPtr<IPropertyHandle>& PropertyHandle,
																   FString& OutFilterString) {
		auto* Property = PropertyHandle->GetProperty();

		if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			// only generate filter string for typed gameplay tags
			const auto* Struct = StructProperty->Struct;
			if (Struct->IsChildOf(FTypedGameplayTag_Base::StaticStruct()))
			{
				FGameplayTagContainer AllRootTags;
				UTypedGameplayTagSettings::GetAllRootTags(OUT AllRootTags, Struct);
				OutFilterString = OUU::Runtime::Private::MakeFilterString(AllRootTags);
				return;
			}
		}

		const auto CategoriesString = UGameplayTagsManager::Get().StaticGetCategoriesMetaFromPropertyHandle(PropertyHandle);
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
			const auto TypedTagStructName = Matches[0].CaptureGroups[1].MatchString;

			for (const auto* Struct : TObjectRange<UScriptStruct>())
			{
				if (Struct->IsChildOf(FTypedGameplayTag_Base::StaticStruct())
					&& Struct->GetName() == TypedTagStructName)
				{
					FGameplayTagContainer AllRootTags;
					UTypedGameplayTagSettings::GetAllRootTags(OUT AllRootTags, Struct);
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
#endif // WITH_EDITOR

FString FTypedGameplayTag_Base::ToShortDisplayString(const FGameplayTag& Tag, const FGameplayTagContainer& RootTags)
{
	FString TagName = Tag.ToString();
	int32 LongestMatch = 0;
	for (const auto& RootTag : RootTags)
	{
		const auto RootTagName = RootTag.ToString() + ".";
		if (TagName.StartsWith(RootTagName, ESearchCase::CaseSensitive))
		{
			LongestMatch = FMath::Max(RootTagName.Len(), LongestMatch);
		}
	}

	TagName.RightChopInline(LongestMatch);

	return TagName;
}