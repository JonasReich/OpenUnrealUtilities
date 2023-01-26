// Copyright (c) 2022 Jonas Reich

#include "DetailsCustomizations/JsonDataAssetPathDetailsCustomization.h"

#include "DetailWidgetRow.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "PropertyHandle.h"
#include "Slate/OUUPropertyCustomizationHelpers.h"

void FJsonDataAssetPathCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto ChildHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJsonDataAssetPath, Path)).ToSharedRef();
	FObjectPropertyBase* ChildProperty = CastField<FObjectPropertyBase>(ChildHandle->GetProperty());

	auto FilterDelegate = FOnShouldFilterAsset::CreateSP(this, &FJsonDataAssetPathCustomization::OnShouldFilterAsset);

	auto EditWidget = OUU::Editor::PropertyCustomizationHelpers::MakeFilteredObjectPropertyWidget(
		ChildHandle,
		ChildProperty,
		CustomizationUtils.GetThumbnailPool(),
		FilterDelegate);

	HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()];
	HeaderRow.ValueContent()[EditWidget];

	OUU::Editor::PropertyCustomizationHelpers::GetClassFiltersFromPropertyMetadata(
		ChildProperty,
		PropertyHandle->GetProperty(),
		OUT AllowedClassFilters,
		OUT DisallowedClassFilters);
}

void FJsonDataAssetPathCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> PropertyHandle,
	IDetailChildrenBuilder& ChildBuilder,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	// do nothing
}

bool FJsonDataAssetPathCustomization::OnShouldFilterAsset(const FAssetData& AssetData) const
{
	auto* AssetClass = AssetData.GetClass();
	{
		bool bAllowedClassFound = false;
		for (auto* AllowClass : AllowedClassFilters)
		{
			if (AssetClass->IsChildOf(AllowClass))
			{
				bAllowedClassFound = true;
				break;
			}
		}
		if (!bAllowedClassFound)
			return true;
	}
	for (auto* DisallowClass : DisallowedClassFilters)
	{
		if (AssetClass->IsChildOf(DisallowClass))
			return true;
	}
	return false;
}
