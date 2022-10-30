// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "ContentBrowserDelegates.h"
#include "IDetailCustomization.h"
#include "IPropertyTypeCustomization.h"

// #TODO-jreich Add parameter if recurse into arrays/structs/maps/sets -> Default true

namespace OUU::Editor::PropertyCustomizationHelpers
{
	/**
	 * Make a widget that filters objects in dropdown based on passed filter delegate.
	 * Required property buttons will be automatically created (e.g. array delete/duplicate)
	 * @returns new widget
	 */
	TSharedRef<SWidget> OUUEDITOR_API MakeFilteredObjectPropertyWidget(
		TSharedRef<IPropertyHandle> PropertyHandle,
		FObjectPropertyBase* ObjectProperty,
		TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
		const FOnShouldFilterAsset& OnShouldFilterAsset);

	/**
	 * Try to make a widget that filters objects in dropdown based on passed filter delegate.
	 * @returns new widget or nullptr, if the passed property is not an object property
	 */
	TSharedPtr<SWidget> OUUEDITOR_API TryMakeFilteredObjectPropertyWidget(
		TSharedRef<IPropertyHandle> PropertyHandle,
		TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
		FOnShouldFilterAsset OnShouldFilterAsset);

	/**
	 * For generate children call in array builders:
	 * Creates filtered object dropdown or default editor for non-object properties.
	 * @returns true if a filter widget was created, false if default
	 */
	bool OUUEDITOR_API MakeFilteredObjectPropertyOrDefault(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildrenBuilder,
		TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
		const FOnShouldFilterAsset& OnShouldFilterAsset);

	/**
	 * For IDetailCustomization:
	 * Makes a filtered object widget and hides default widget OR does nothing (i.e. leaves default as is).
	 * @returns if a filter widget was created, false if default
	 */
	bool OUUEDITOR_API TryOverrideFilteredObjectPropertyWidget(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailLayoutBuilder& DetailBuilder,
		const FOnShouldFilterAsset& OnShouldFilterAsset);

	/**
	 * For IPropertyTypeCustomization:
	 * Call in CustomizeChildren() to filter all object properties.
	 * Disadvantage: No customization of non-object properties in recursed arrays/structs after-the-fact.
	 */
	void OUUEDITOR_API CustomizeChildren_FilterObjectProperties(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils,
		const FOnShouldFilterAsset& OnShouldFilterAsset);

	/**
	 * For IDetailCustomization:
	 * Call in CustomizeDetails() to filter all object properties.
	 * Disadvantage: No customization of non-object properties possible after-the-fact.
	 */
	void OUUEDITOR_API CustomizeDetails_FilterObjectProperties(
		IDetailLayoutBuilder& DetailBuilder,
		UClass* OwningClass,
		const FOnShouldFilterAsset& OnShouldFilterAsset);

} // namespace OUU::Editor::PropertyCustomizationHelpers
