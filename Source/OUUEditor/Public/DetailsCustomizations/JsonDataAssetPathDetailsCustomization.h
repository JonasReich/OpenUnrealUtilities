// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "IPropertyTypeCustomization.h"

class FJsonDataAssetPathCustomization : public IPropertyTypeCustomization
{
public:
	// - IPropertyTypeCustomization
	void CustomizeHeader(
		TSharedRef<IPropertyHandle> PropertyHandle,
		FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	void CustomizeChildren(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	// --

private:
	TArray<const UClass*> AllowedClassFilters;
	TArray<const UClass*> DisallowedClassFilters;

	bool OnShouldFilterAsset(const FAssetData& AssetData) const;
};
