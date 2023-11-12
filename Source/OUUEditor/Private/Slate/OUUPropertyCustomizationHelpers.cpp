// Copyright (c) 2023 Jonas Reich & Contributors

#include "Slate/OUUPropertyCustomizationHelpers.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IContentBrowserSingleton.h"
#include "IDetailChildrenBuilder.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "LogOpenUnrealUtilities.h"
#include "ObjectEditorUtils.h"
#include "UObject/UObjectIterator.h"

namespace OUU::Editor::PropertyCustomizationHelpers
{
	namespace Private
	{
		// ReSharper disable CppPassValueParameterByConstReference
		void OnGenerateElementForObjectArray(
			TSharedRef<IPropertyHandle> PropertyHandle,
			int32 GroupIndex,
			IDetailChildrenBuilder& ChildrenBuilder,
			TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
			FOnShouldFilterAsset OnShouldFilterAsset)
		// ReSharper restore CppPassValueParameterByConstReference
		{
			OUU::Editor::PropertyCustomizationHelpers::MakeFilteredObjectPropertyOrDefault(
				PropertyHandle,
				ChildrenBuilder,
				ThumbnailPool,
				OnShouldFilterAsset);
		}

		// Helper to support both meta=(TagName) and meta=(TagName=true) syntaxes
		static bool GetTagOrBoolMetadata(const FProperty* Property, const TCHAR* TagName, bool bDefault)
		{
			bool bResult = bDefault;

			if (Property->HasMetaData(TagName))
			{
				bResult = true;

				const FString ValueString = Property->GetMetaData(TagName);
				if (!ValueString.IsEmpty())
				{
					if (ValueString == TEXT("true"))
					{
						bResult = true;
					}
					else if (ValueString == TEXT("false"))
					{
						bResult = false;
					}
				}
			}

			return bResult;
		}
	} // namespace Private

	TSharedRef<SObjectPropertyEntryBox> MakeFilteredObjectPropertyWidget(
		const TSharedRef<IPropertyHandle>& PropertyHandle,
		const FObjectPropertyBase* ObjectProperty,
		const TSharedPtr<FAssetThumbnailPool>& ThumbnailPool,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		return SNew(SObjectPropertyEntryBox)
			.ThumbnailPool(ThumbnailPool)
			.PropertyHandle(PropertyHandle)
			.AllowedClass(ObjectProperty->PropertyClass)
			.AllowClear(true)
			.OnShouldFilterAsset(OnShouldFilterAsset);
	}

	TSharedPtr<SObjectPropertyEntryBox> TryMakeFilteredObjectPropertyWidget(
		const TSharedRef<IPropertyHandle>& PropertyHandle,
		const TSharedPtr<FAssetThumbnailPool>& ThumbnailPool,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		if (const auto* ObjectProperty = CastField<FObjectPropertyBase>(PropertyHandle->GetProperty()))
		{
			return MakeFilteredObjectPropertyWidget(PropertyHandle, ObjectProperty, ThumbnailPool, OnShouldFilterAsset);
		}
		return nullptr;
	}

	bool MakeFilteredObjectPropertyOrDefault(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildrenBuilder,
		const TSharedPtr<FAssetThumbnailPool>& ThumbnailPool,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		// references:
		// - FMetasoundDefaultMemberElementDetailCustomizationBase::CustomizeChildren
		// - FMetasoundDefaultMemberElementDetailCustomizationBase::CreateValueWidget

		const auto PropWidget = TryMakeFilteredObjectPropertyWidget(PropertyHandle, ThumbnailPool, OnShouldFilterAsset);
		if (!PropWidget)
		{
			// Ideally we would recurse into array/struct properties here, but that's super hard.
			// Instead we register customization using the children builder for every supported nested struct type.
			ChildrenBuilder.AddProperty(PropertyHandle);
			return false;
		}

		TSharedPtr<IPropertyHandleArray> ParentPropertyHandleArray;
		TSharedPtr<IPropertyHandle> ElementPropertyHandle = PropertyHandle;
		if (ElementPropertyHandle.IsValid())
		{
			const TSharedPtr<IPropertyHandle> ParentProperty = ElementPropertyHandle->GetParentHandle();
			while (ParentProperty.IsValid() && ParentProperty->GetProperty() != nullptr)
			{
				ParentPropertyHandleArray = ParentProperty->AsArray();
				if (ParentPropertyHandleArray.IsValid())
				{
					ElementPropertyHandle = ParentProperty;
					break;
				}
			}
		}

		TSharedPtr<IPropertyHandle> StructPropertyPtr = PropertyHandle;
		const FExecuteAction InsertAction =
			FExecuteAction::CreateLambda([ParentPropertyHandleArray, StructPropertyPtr] {
				const int32 ArrayIndex =
					StructPropertyPtr.IsValid() ? StructPropertyPtr->GetIndexInArray() : INDEX_NONE;
				if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
				{
					ParentPropertyHandleArray->Insert(ArrayIndex);
				}
			});

		const FExecuteAction DeleteAction =
			FExecuteAction::CreateLambda([ParentPropertyHandleArray, StructPropertyPtr] {
				const int32 ArrayIndex =
					StructPropertyPtr.IsValid() ? StructPropertyPtr->GetIndexInArray() : INDEX_NONE;
				if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
				{
					ParentPropertyHandleArray->DeleteItem(ArrayIndex);
				}
			});

		const FExecuteAction DuplicateAction =
			FExecuteAction::CreateLambda([ParentPropertyHandleArray, StructPropertyPtr] {
				const int32 ArrayIndex =
					StructPropertyPtr.IsValid() ? StructPropertyPtr->GetIndexInArray() : INDEX_NONE;
				if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
				{
					ParentPropertyHandleArray->DuplicateItem(ArrayIndex);
				}
			});
		const auto PropertyNameWidget = PropertyHandle->CreatePropertyNameWidget();

		// clang-format off
		const auto ValueWidgetWrapper =
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(1.0f)
			.Padding(1.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				PropWidget.ToSharedRef()
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			// Negative padding intentional on the left to bring the dropdown
			// closer to the other buttons
			.Padding(-6.0f, 0.0f, 0.0f, 0.0f)
			.VAlign(VAlign_Center)
			[
				::PropertyCustomizationHelpers::MakeInsertDeleteDuplicateButton(InsertAction,
																		DeleteAction,
																		DuplicateAction)
			];
		// clang-format on

		// clang-format off
		ChildrenBuilder.AddCustomRow(PropertyHandle->GetPropertyDisplayName())
			.Visibility(EVisibility::Visible)
			.NameContent()
			[
				PropertyNameWidget
			]
			.ValueContent()
			.MinDesiredWidth(600)
			.MaxDesiredWidth(600)
			[
				ValueWidgetWrapper
			]
			.PropertyHandleList({PropertyHandle});
		// clang-format on

		return true;
	}

	bool TryOverrideFilteredObjectPropertyWidget(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailLayoutBuilder& DetailBuilder,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		if (!PropertyHandle->IsValidHandle())
			return false;

		const auto* Field = PropertyHandle->GetProperty();

		IDetailCategoryBuilder& PropertyCategory =
			DetailBuilder.EditCategory(FObjectEditorUtils::GetCategoryFName(Field));

		if (auto pArrayHandle = PropertyHandle->AsArray())
		{
			const auto ArrayBuilder = MakeShared<FDetailArrayBuilder>(PropertyHandle);
			ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateStatic(
				&OUU::Editor::PropertyCustomizationHelpers::Private::OnGenerateElementForObjectArray,
				DetailBuilder.GetThumbnailPool(),
				FOnShouldFilterAsset(OnShouldFilterAsset)));

			PropertyCategory.AddCustomBuilder(ArrayBuilder);
			DetailBuilder.HideProperty(PropertyHandle);
			return true;
		}

		if (const auto PropWidget = OUU::Editor::PropertyCustomizationHelpers::TryMakeFilteredObjectPropertyWidget(
				PropertyHandle,
				DetailBuilder.GetThumbnailPool(),
				OnShouldFilterAsset))
		{
			PropertyCategory.AddCustomRow(PropertyHandle->GetPropertyDisplayName())
				.Visibility(EVisibility::Visible)
				.NameContent()[PropertyHandle->CreatePropertyNameWidget()]
				.ValueContent()
				.MinDesiredWidth(600)
				.MaxDesiredWidth(600)[PropWidget.ToSharedRef()]
				.PropertyHandleList({PropertyHandle});

			DetailBuilder.HideProperty(PropertyHandle);
			return true;
		}
		else
		{
			// theoretically recurse into children?
		}

		return false;
	}

	void CustomizeChildren_FilterObjectProperties(
		const TSharedRef<IPropertyHandle>& PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		const IPropertyTypeCustomizationUtils& CustomizationUtils,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		auto ThumbnailPool = CustomizationUtils.GetThumbnailPool();

		uint32 NumChildProps = 0;
		PropertyHandle->GetNumChildren(OUT NumChildProps);
		for (uint32 i = 0; i < NumChildProps; i++)
		{
			auto ChildHandle = PropertyHandle->GetChildHandle(i).ToSharedRef();

			if (auto pArrayHandle = ChildHandle->AsArray())
			{
				// Doesn't work for TMaps!
				// -> see FSonyPackagingSettingsDetails_ContentIDBuilder for a TMap customization sample
				auto ArrayBuilder = MakeShared<FDetailArrayBuilder>(ChildHandle);
				ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateStatic(
					&OUU::Editor::PropertyCustomizationHelpers::Private::OnGenerateElementForObjectArray,
					ThumbnailPool,
					FOnShouldFilterAsset(OnShouldFilterAsset)));

				ChildBuilder.AddCustomBuilder(ArrayBuilder);
				continue;
			}

			if (auto PropWidget = OUU::Editor::PropertyCustomizationHelpers::TryMakeFilteredObjectPropertyWidget(
					ChildHandle,
					ThumbnailPool,
					OnShouldFilterAsset))
			{
				ChildBuilder.AddCustomRow(ChildHandle->GetPropertyDisplayName())
					.Visibility(EVisibility::Visible)
					.NameContent()[ChildHandle->CreatePropertyNameWidget()]
					.ValueContent()
					.MinDesiredWidth(600)
					.MaxDesiredWidth(600)[PropWidget.ToSharedRef()]
					.PropertyHandleList({ChildHandle});
			}
			else
			{
				ChildBuilder.AddProperty(ChildHandle);
			}
		}
	}

	void CustomizeDetails_FilterObjectProperties(
		IDetailLayoutBuilder& DetailBuilder,
		UClass* OwningClass,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		for (const FProperty* Property : TFieldRange<FProperty>(OwningClass))
		{
			if ((Property->PropertyFlags & EPropertyFlags::CPF_Edit) == 0)
			{
				continue;
			}

			const TSharedRef<IPropertyHandle> PropertyHandle =
				DetailBuilder.GetProperty(Property->GetFName(), OwningClass);

			OUU::Editor::PropertyCustomizationHelpers::TryOverrideFilteredObjectPropertyWidget(
				PropertyHandle,
				DetailBuilder,
				OnShouldFilterAsset);
		}
	}

	void GetClassFiltersFromPropertyMetadata(
		const FProperty* Property,
		const FProperty* MetadataProperty,
		TArray<const UClass*>& OutAllowedClassFilters,
		TArray<const UClass*>& OutDisallowedClassFilters)
	{
		const auto* ObjectClass = GetObjectPropertyClass(Property);

		// Copied from void SPropertyEditorAsset::InitializeClassFilters(const FProperty* Property)
		if (Property == nullptr)
		{
			OutAllowedClassFilters.Add(ObjectClass);
			return;
		}

		bool bExactClass = Private::GetTagOrBoolMetadata(MetadataProperty, TEXT("ExactClass"), false);

		auto FindClass = [](const FString& InClassName) {
			UClass* Class = UClass::TryFindTypeSlow<UClass>(InClassName, EFindFirstObjectOptions::EnsureIfAmbiguous);
			if (!Class)
			{
				Class = LoadObject<UClass>(nullptr, *InClassName);
			}
			return Class;
		};

		const FString AllowedClassesFilterString = MetadataProperty->GetMetaData(TEXT("AllowedClasses"));
		if (!AllowedClassesFilterString.IsEmpty())
		{
			TArray<FString> AllowedClassFilterNames;
			AllowedClassesFilterString.ParseIntoArrayWS(AllowedClassFilterNames, TEXT(","), true);

			for (const FString& ClassName : AllowedClassFilterNames)
			{
				if (const UClass* Class = FindClass(ClassName))
				{
					// If the class is an interface, expand it to be all classes in memory that implement the class.
					if (Class->HasAnyClassFlags(CLASS_Interface))
					{
						for (const UClass* ClassWithInterface : TObjectRange<UClass>())
						{
							if (ClassWithInterface->ImplementsInterface(Class))
							{
								OutAllowedClassFilters.Add(ClassWithInterface);
							}
						}
					}
					else
					{
						OutAllowedClassFilters.Add(Class);
					}
				}
			}
		}

		if (OutAllowedClassFilters.Num() == 0)
		{
			// always add the object class to the filters if it was not further filtered
			OutAllowedClassFilters.Add(ObjectClass);
		}

		const FString DisallowedClassesFilterString = MetadataProperty->GetMetaData(TEXT("DisallowedClasses"));
		if (!DisallowedClassesFilterString.IsEmpty())
		{
			TArray<FString> DisallowedClassFilterNames;
			DisallowedClassesFilterString.ParseIntoArrayWS(DisallowedClassFilterNames, TEXT(","), true);

			for (const FString& ClassName : DisallowedClassFilterNames)
			{
				if (const UClass* Class = FindClass(ClassName))
				{
					// If the class is an interface, expand it to be all classes in memory that implement the class.
					if (Class->HasAnyClassFlags(CLASS_Interface))
					{
						for (const UClass* ClassWithInterface : TObjectRange<UClass>())
						{
							if (ClassWithInterface->ImplementsInterface(Class))
							{
								OutDisallowedClassFilters.Add(ClassWithInterface);
							}
						}
					}
					else
					{
						OutDisallowedClassFilters.Add(Class);
					}
				}
			}
		}
	}

	UClass* GetObjectPropertyClass(const FProperty* Property)
	{
		UClass* Class = nullptr;

		if (CastField<const FObjectPropertyBase>(Property) != nullptr)
		{
			Class = CastField<const FObjectPropertyBase>(Property)->PropertyClass;
			if (Class == nullptr)
			{
				UE_LOG(
					LogOpenUnrealUtilities,
					Warning,
					TEXT("Object Property (%s) has a null class, falling back to UObject"),
					*Property->GetFullName());
				Class = UObject::StaticClass();
			}
		}
		else if (CastField<const FInterfaceProperty>(Property) != nullptr)
		{
			Class = CastField<const FInterfaceProperty>(Property)->InterfaceClass;
			if (Class == nullptr)
			{
				UE_LOG(
					LogOpenUnrealUtilities,
					Warning,
					TEXT("Interface Property (%s) has a null class, falling back to UObject"),
					*Property->GetFullName());
				Class = UObject::StaticClass();
			}
		}
		else
		{
			ensureMsgf(
				Class != nullptr,
				TEXT("Property (%s) is not an object or interface class"),
				Property ? *Property->GetFullName() : TEXT("null"));
			Class = UObject::StaticClass();
		}
		return Class;
	}

} // namespace OUU::Editor::PropertyCustomizationHelpers
