// Copyright (c) 2022 Jonas Reich

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
	auto JsonContentRoot = OUU::Runtime::JsonData::GetCacheMountPointRoot_Package();

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

	for (auto& FieldAndValue : TPropertyValueRange<FObjectPropertyBase>(InAsset->GetClass(), InAsset))
	{
		auto& Field = FieldAndValue.Key;
		auto& ValuePtr = FieldAndValue.Value;

		if (Field == PathProperty)
		{
			// Ignore the json data path property
			continue;
		}

		UObject* Object = Field->LoadObjectPropertyValue(ValuePtr);
		if (!IsValid(Object))
		{
			// Skip null object properties
			continue;
		}

		if (Object->IsA<UJsonDataAsset>())
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
