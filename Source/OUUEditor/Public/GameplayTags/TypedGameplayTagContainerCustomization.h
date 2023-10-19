// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "GameplayTagContainer.h"
#include "GameplayTags/TypedGameplayTagContainer.h"
#include "IPropertyTypeCustomization.h"

class SBox;

class FTypedGameplayTagContainer_PropertyTypeCustomization : public IPropertyTypeCustomization
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
	void RefreshContainerWidget();

	void SetNewTags(const FGameplayTagContainer& NewValue);

private:
	TArray<FName> TypedTagOptions;
	TSharedPtr<FGameplayTagContainer> WorkingContainer;
	TSharedPtr<SBox> GameplayTagContainerBox;
	TSharedPtr<SBox> ErrorBox;
	TSharedPtr<IPropertyHandle> HandleSP;
};
