// Copyright (c) 2022 Jonas Reich

#include "Slate/OUUPropertyCustomizationHelpers.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "IContentBrowserSingleton.h"
#include "IDetailChildrenBuilder.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"
#include "ObjectEditorUtils.h"
#include "PropertyCustomizationHelpers.h"

namespace OUU::Editor::PropertyCustomizationHelpers
{
	namespace Private
	{
		void OnGenerateElementForObjectArray(
			TSharedRef<IPropertyHandle> PropertyHandle,
			int32 GroupIndex,
			IDetailChildrenBuilder& ChildrenBuilder,
			TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
			FOnShouldFilterAsset OnShouldFilterAsset)
		{
			OUU::Editor::PropertyCustomizationHelpers::MakeFilteredObjectPropertyOrDefault(
				PropertyHandle,
				ChildrenBuilder,
				ThumbnailPool,
				OnShouldFilterAsset);
		}
	} // namespace Private

	TSharedRef<SWidget> MakeFilteredObjectPropertyWidget(
		TSharedRef<IPropertyHandle> PropertyHandle,
		FObjectPropertyBase* ObjectProperty,
		TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		return SNew(SObjectPropertyEntryBox)
			.ThumbnailPool(ThumbnailPool)
			.PropertyHandle(PropertyHandle)
			.AllowedClass(ObjectProperty->PropertyClass)
			.AllowClear(true)
			.OnShouldFilterAsset(OnShouldFilterAsset);
	}

	TSharedPtr<SWidget> TryMakeFilteredObjectPropertyWidget(
		TSharedRef<IPropertyHandle> PropertyHandle,
		TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
		FOnShouldFilterAsset OnShouldFilterAsset)
	{
		if (auto* ObjectProperty = CastField<FObjectPropertyBase>(PropertyHandle->GetProperty()))
		{
			return MakeFilteredObjectPropertyWidget(PropertyHandle, ObjectProperty, ThumbnailPool, OnShouldFilterAsset);
		}
		return nullptr;
	}

	bool MakeFilteredObjectPropertyOrDefault(
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildrenBuilder,
		TSharedPtr<FAssetThumbnailPool> ThumbnailPool,
		const FOnShouldFilterAsset& OnShouldFilterAsset)
	{
		// references:
		// - FMetasoundDefaultMemberElementDetailCustomizationBase::CustomizeChildren
		// - FMetasoundDefaultMemberElementDetailCustomizationBase::CreateValueWidget

		auto PropWidget = TryMakeFilteredObjectPropertyWidget(PropertyHandle, ThumbnailPool, OnShouldFilterAsset);
		if (!PropWidget)
		{
			// Ideally we would recurse into array/struct properties here, but that's super hard.
			// Instead we register FGrimAnimDataAsset_NestedStruct_PropertyTypeCustomization
			// for every supported nested struct type.
			ChildrenBuilder.AddProperty(PropertyHandle);
			return false;
		}

		TSharedPtr<IPropertyHandleArray> ParentPropertyHandleArray;
		TSharedPtr<IPropertyHandle> ElementPropertyHandle = PropertyHandle;
		if (ElementPropertyHandle.IsValid())
		{
			TSharedPtr<IPropertyHandle> ParentProperty = ElementPropertyHandle->GetParentHandle();
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
		FExecuteAction InsertAction = FExecuteAction::CreateLambda([ParentPropertyHandleArray, StructPropertyPtr] {
			const int32 ArrayIndex = StructPropertyPtr.IsValid() ? StructPropertyPtr->GetIndexInArray() : INDEX_NONE;
			if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
			{
				ParentPropertyHandleArray->Insert(ArrayIndex);
			}
		});

		FExecuteAction DeleteAction = FExecuteAction::CreateLambda([ParentPropertyHandleArray, StructPropertyPtr] {
			const int32 ArrayIndex = StructPropertyPtr.IsValid() ? StructPropertyPtr->GetIndexInArray() : INDEX_NONE;
			if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
			{
				ParentPropertyHandleArray->DeleteItem(ArrayIndex);
			}
		});

		FExecuteAction DuplicateAction = FExecuteAction::CreateLambda([ParentPropertyHandleArray, StructPropertyPtr] {
			const int32 ArrayIndex = StructPropertyPtr.IsValid() ? StructPropertyPtr->GetIndexInArray() : INDEX_NONE;
			if (ParentPropertyHandleArray.IsValid() && ArrayIndex >= 0)
			{
				ParentPropertyHandleArray->DuplicateItem(ArrayIndex);
			}
		});
		auto PropertyNameWidget = PropertyHandle->CreatePropertyNameWidget();

		// clang-format off
		auto ValueWidgetWrapper =
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

		auto* Field = PropertyHandle->GetProperty();

		IDetailCategoryBuilder& PropertyCategory =
			DetailBuilder.EditCategory(FObjectEditorUtils::GetCategoryFName(Field));

		if (auto pArrayHandle = PropertyHandle->AsArray())
		{
			auto ArrayBuilder = MakeShared<FDetailArrayBuilder>(PropertyHandle);
			ArrayBuilder->OnGenerateArrayElementWidget(FOnGenerateArrayElementWidget::CreateStatic(
				&OUU::Editor::PropertyCustomizationHelpers::Private::OnGenerateElementForObjectArray,
				DetailBuilder.GetThumbnailPool(),
				FOnShouldFilterAsset(OnShouldFilterAsset)));

			PropertyCategory.AddCustomBuilder(ArrayBuilder);
			DetailBuilder.HideProperty(PropertyHandle);
			return true;
		}

		if (auto PropWidget = OUU::Editor::PropertyCustomizationHelpers::TryMakeFilteredObjectPropertyWidget(
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
		TSharedRef<IPropertyHandle> PropertyHandle,
		IDetailChildrenBuilder& ChildBuilder,
		IPropertyTypeCustomizationUtils& CustomizationUtils,
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
		for (FProperty* Property : TFieldRange<FProperty>(OwningClass))
		{
			if ((Property->PropertyFlags & EPropertyFlags::CPF_Edit) == 0)
			{
				continue;
			}

			TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(Property->GetFName(), OwningClass);

			OUU::Editor::PropertyCustomizationHelpers::TryOverrideFilteredObjectPropertyWidget(
				PropertyHandle,
				DetailBuilder,
				OnShouldFilterAsset);
		}
	}
} // namespace OUU::Editor::PropertyCustomizationHelpers
