// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/AssetValidator_JsonDataAssetReferences.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"
#include "JsonDataAsset/JsonDataAsset.h"

bool UAssetValidator_JsonDataAssetReferences::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return IsValid(InAsset);
}

EDataValidationResult UAssetValidator_JsonDataAssetReferences::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
{
	// Skip assets that have no references to the json data folder as per the asset registry
	// This should be a lot faster than loading all object dependencies as most objects will not have dependencies to
	// json content.
	if (HasJsonDependency(InAsset))
	{
		UObject* ObjectToValidate = InAsset;
		if (auto* Blueprint = Cast<UBlueprint>(InAsset))
		{
			// Blueprint property values are on the CDO of the generated class
			ObjectToValidate = Blueprint->GeneratedClass->GetDefaultObject();
		}
		return ValidateLoadedAsset_Impl(ObjectToValidate, OUT ValidationErrors);
	}

	AssetPasses(InAsset);
	return EDataValidationResult::Valid;
}

bool UAssetValidator_JsonDataAssetReferences::HasJsonDependency(UObject* InAsset)
{
	// The game root is also the virtual root of plugin content, so we don't have to iterate over plugin roots
	// (this is rarely the case!)
	auto JsonContentRoot = OUU::Runtime::JsonData::GetCacheMountPointRoot_Package(OUU::Runtime::JsonData::GameRootName);

	TArray<FAssetIdentifier> Dependencies;
	IAssetRegistry::Get()->GetDependencies(FAssetIdentifier(InAsset->GetOutermost()->GetFName()), OUT Dependencies);
	for (auto& Dependency : Dependencies)
	{
		if (Dependency.PackageName.ToString().Contains(JsonContentRoot))
		{
			return true;
		}
	}

	return false;
}

EDataValidationResult UAssetValidator_JsonDataAssetReferences::ValidateLoadedAsset_Impl(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
{
	if (IsValid(InAsset) == false)
	{
		AssetPasses(InAsset);
		return EDataValidationResult::Valid;
	}

	// This is the only property allowed to have references to json content
	auto PathProperty =
		FJsonDataAssetPath::StaticStruct()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(FJsonDataAssetPath, Path));

	// Again we only really care if it's in _any_ json content root, which are all underneath the game root.
	auto JsonContentRoot = OUU::Runtime::JsonData::GetCacheMountPointRoot_Package(OUU::Runtime::JsonData::GameRootName);

	for (auto& FieldAndValue : TPropertyValueRange<FObjectPropertyBase>(InAsset->GetClass(), InAsset))
	{
		auto& Field = FieldAndValue.Key;
		auto& ValuePtr = FieldAndValue.Value;

		if (Field == PathProperty || Field->HasAnyPropertyFlags(CPF_Transient)
			|| Field->IsA(FWeakObjectProperty::StaticClass()))
		{
			// Skip anything that either is a valid FJsonDataAssetPath property or that will not actually be saved
			continue;
		}

		if (Field->PropertyClass == nullptr
			|| (Field->PropertyClass->IsChildOf<UJsonDataAsset>() == false
				&& UJsonDataAsset::StaticClass()->IsChildOf(Field->PropertyClass) == false))
		{
			// Skip fields that cannot contain json data assets
			continue;
		}

		UObject* Object = Field->GetObjectPropertyValue(ValuePtr);
		bool bIsReferencingJsonAsset = false;
		if (IsValid(Object))
		{
			bIsReferencingJsonAsset = Object->IsA<UJsonDataAsset>();
		}
		else
		{
			const auto SoftObjectPtrField = CastField<FSoftObjectProperty>(Field);
			if (SoftObjectPtrField)
			{
				const auto& SoftObjectPtr = SoftObjectPtrField->GetPropertyValue(ValuePtr);
				bIsReferencingJsonAsset = SoftObjectPtr.GetLongPackageName().Contains(JsonContentRoot);
			}
		}

		if (bIsReferencingJsonAsset)
		{
			AssetFails(
				InAsset,
				FText::FromString(FString::Printf(
					TEXT("Property %s contains a direct reference to json data asset %s. Please only use "
						 "FJsonDataAssetPath to reference json data assets!"),
					*Field->GetFullName(),
					*Object->GetName())),
				OUT ValidationErrors);
			return EDataValidationResult::Invalid;
		}
	}

	AssetPasses(InAsset);
	return EDataValidationResult::Valid;
}
