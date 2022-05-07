// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Modules/ModuleManager.h"
#include "PropertyEditorModule.h"

namespace OUU::Editor::PropertyEditorUtils
{
	namespace Private
	{
		FORCEINLINE FPropertyEditorModule& GetPropertyEditorChecked()
		{
			return FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		}

		FORCEINLINE FPropertyEditorModule* GetPropertyEditor()
		{
			return FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");
		}

		template <typename T>
		typename TEnableIf<TIsDerivedFrom<T, UObject>::Value, FName>::Type GetStaticTypeName()
		{
			return T::StaticClass()->GetFName();
		}

		template <typename T>
		typename TEnableIf<TIsDerivedFrom<T, UObject>::Value == false, FName>::Type GetStaticTypeName()
		{
			return T::StaticStruct()->GetFName();
		}
	} // namespace Private

	/**
	 * Registers a property type layout customization
	 * A property type is a specific FProperty type, a struct, or enum type
	 * @tparam TargetType is the type for which the property layout is being customized
	 * @tparam LayoutType is the type that implements the IPropertyTypeCustomization interface
	 */
	template <typename TargetType, typename LayoutType>
	void RegisterCustomPropertyTypeLayout()
	{
		static_assert(
			TIsDerivedFrom<LayoutType, IPropertyTypeCustomization>::Value,
			"LayoutType must be a property type customization class");

		Private::GetPropertyEditorChecked().RegisterCustomPropertyTypeLayout(
			Private::GetStaticTypeName<TargetType>(),
			FOnGetPropertyTypeCustomizationInstance::CreateLambda(
				[]() -> TSharedRef<IPropertyTypeCustomization> { return MakeShared<LayoutType>(); }));
	}

	/**
	 * Unregisters a custom property layout for a property type
	 * @tparam TargetType is the type for which the property layout was registered
	 */
	template <typename TargetType>
	void UnregisterCustomPropertyTypeLayout()
	{
		if (auto* PropertyEditor = Private::GetPropertyEditor())
		{
			PropertyEditor->UnregisterCustomPropertyTypeLayout(Private::GetStaticTypeName<TargetType>());
		}
	}
	/**
	 * Registers a custom detail layout delegate for a specific class
	 * @tparam TargetType is the type for which the class layout is being customized
	 * @tparam LayoutType is the type that implements the IDetailCustomization interface
	 */
	template <typename TargetType, typename LayoutType>
	void RegisterCustomClassLayout()
	{
		static_assert(
			TIsDerivedFrom<LayoutType, IDetailCustomization>::Value,
			"LayoutType must be a detail customization class");

		Private::GetPropertyEditorChecked().RegisterCustomClassLayout(
			Private::GetStaticTypeName<TargetType>(),
			FOnGetDetailCustomizationInstance::CreateLambda(
				[]() -> TSharedRef<IDetailCustomization> { return MakeShared<LayoutType>(); }));
	}

	/**
	 * Unregisters a custom detail layout delegate for a specific class name
	 * @tparam TargetType is the type for which the class layout was registered
	 */
	template <typename TargetType>
	void UnregisterCustomClassLayout()
	{
		if (auto* PropertyEditor = Private::GetPropertyEditor())
		{
			PropertyEditor->UnregisterCustomClassLayout(Private::GetStaticTypeName<TargetType>());
		}
	}

} // namespace OUU::Editor::PropertyEditorUtils
