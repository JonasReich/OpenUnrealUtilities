// Copyright (c) 2022 Jonas Reich

#include "DetailsCustomizations/JsonDataAssetPathDetailsCustomization.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "ContentBrowserDataDragDropOp.h"
#include "ContentBrowserModule.h"
#include "DetailWidgetRow.h"
#include "Engine/AssetManager.h"
#include "IContentBrowserSingleton.h"
#include "Input/DragAndDrop.h"
#include "JsonDataAsset/JsonAssetReferenceFilter.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "JsonDataAsset/JsonDataAssetEditor.h"
#include "PropertyCustomizationHelpers.h"
#include "PropertyHandle.h"
#include "SDropTarget.h"
#include "Slate/OUUPropertyCustomizationHelpers.h"

namespace OUU::Editor::JsonData::Private
{
	TOptional<FJsonDataAssetPath> GetFirstJsonPathFromDragDropOp(
		TSharedPtr<FContentBrowserDataDragDropOp> ContentDragDropOp)
	{
		auto Files = ContentDragDropOp->GetDraggedFiles();
		auto Prefix = TEXT("/All") + OUU::Runtime::JsonData::GetSourceMountPointRoot_Package();
		for (auto File : Files)
		{
			if (auto* FileItem = File.GetPrimaryInternalItem())
			{
				auto VirtualPath = FileItem->GetVirtualPath().ToString();
				if (VirtualPath.StartsWith(Prefix))
				{
					VirtualPath.RemoveFromStart(TEXT("/All"));
					FJsonDataAssetPath Result =
						OUU::Editor::JsonData::ConvertMountedSourceFilenameToDataAssetPath(VirtualPath);
					return Result;
				}
			}
		}
		return {};
	}
} // namespace OUU::Editor::JsonData::Private

void FJsonDataAssetPathCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> PropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	auto ChildHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FJsonDataAssetPath, Path)).ToSharedRef();
	FObjectPropertyBase* ChildProperty = CastField<FObjectPropertyBase>(ChildHandle->GetProperty());

	TArray<UObject*> OuterObjects;
	PropertyHandle->GetOuterObjects(OuterObjects);

	OwningAssets.Empty();
	auto& AssetManager = UAssetManager::Get();
	for (auto* OuterObject : OuterObjects)
	{
		FAssetData Data;
		if (AssetManager.GetAssetDataForPath(OuterObject->GetPathName(), OUT Data))
		{
			OwningAssets.Add(Data);
		}
	}

	auto FilterDelegate = FOnShouldFilterAsset::CreateSP(this, &FJsonDataAssetPathCustomization::OnShouldFilterAsset);

	TArray<FAssetData> ContextOwnerAssets{FJsonAssetReferenceFilter::PassFilterKey()};
	auto EditWidget = SNew(SObjectPropertyEntryBox)
						  .ThumbnailPool(CustomizationUtils.GetThumbnailPool())
						  .PropertyHandle(ChildHandle)
						  .AllowedClass(ChildProperty->PropertyClass)
						  .AllowClear(true)
						  .OnShouldFilterAsset(FilterDelegate)
						  .OwnerAssetDataArray(ContextOwnerAssets);

	auto IsRecognized = [](TSharedPtr<FDragDropOperation> DragDropOperation) -> bool {
		if (auto ContentBrowserDragDropOp = StaticCastSharedPtr<FContentBrowserDataDragDropOp>(DragDropOperation))
		{
			auto OptJsonPath = OUU::Editor::JsonData::Private::GetFirstJsonPathFromDragDropOp(ContentBrowserDragDropOp);
			return OptJsonPath.IsSet();
		}
		return false;
	};

	auto AllowDrop = [this](TSharedPtr<FDragDropOperation> DragDropOperation) {
		if (auto ContentBrowserDragDropOp = StaticCastSharedPtr<FContentBrowserDataDragDropOp>(DragDropOperation))
		{
			auto OptJsonPath = OUU::Editor::JsonData::Private::GetFirstJsonPathFromDragDropOp(ContentBrowserDragDropOp);

			if (OptJsonPath.IsSet())
			{
				auto JsonPackagePath = OptJsonPath->GetPackagePath();
				auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(JsonPackagePath);

				auto AssetData = IAssetRegistry::Get()->GetAssetByObjectPath(
					FSoftObjectPath(JsonPackagePath + TEXT(".") + ObjectName));

				// Allow dropping if the filter would not filter the asset out
				return this->OnShouldFilterAsset(AssetData) == false;
			}
		}
		return false;
	};

	auto OnDroppedLambda = [PropertyHandle](const FGeometry&, const FDragDropEvent& DragDropEvent) -> FReply {
		if (TSharedPtr<FContentBrowserDataDragDropOp> ContentDragDropOp =
				DragDropEvent.GetOperationAs<FContentBrowserDataDragDropOp>())
		{
			auto OptJsonPath = OUU::Editor::JsonData::Private::GetFirstJsonPathFromDragDropOp(ContentDragDropOp);
			if (OptJsonPath.IsSet())
			{
				auto JsonPackagePath = OptJsonPath->GetPackagePath();
				auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(JsonPackagePath);
				PropertyHandle->SetValueFromFormattedString(JsonPackagePath + TEXT(".") + ObjectName);
				return FReply::Handled();
			}
		}
		return FReply::Unhandled();
	};

	auto CustomJsonDataDropTarget = SNew(SDropTarget)
										.OnIsRecognized(SDropTarget::FVerifyDrag::CreateLambda(IsRecognized))
										.OnAllowDrop(SDropTarget::FVerifyDrag::CreateLambda(AllowDrop))
										.OnDropped(FOnDrop::CreateLambda(OnDroppedLambda))
										.Content()[EditWidget];

	HeaderRow.NameContent()[PropertyHandle->CreatePropertyNameWidget()];
	HeaderRow.ValueContent()[CustomJsonDataDropTarget];

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
	// Blueprint based classes may not be loaded yet, so we need to load it manually
	if (AssetClass == nullptr)
	{
		AssetClass = FSoftClassPath(AssetData.AssetClassPath.ToString()).TryLoadClass<UObject>();
	}

	if (AssetClass)
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
		for (auto* DisallowClass : DisallowedClassFilters)
		{
			if (AssetClass->IsChildOf(DisallowClass))
				return true;
		}
	}
	return false;
}
