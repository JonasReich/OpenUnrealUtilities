// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUEditorSubsystem.h"

#include "DetailsCustomizations/JsonDataAssetPathDetailsCustomization.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "PropertyEditorUtils.h"
#include "UObject/UObjectIterator.h"

void UOUUEditorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	auto& PropertyEditor = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	for (TObjectIterator<UStruct> It; It; ++It)
	{
		if (It->IsChildOf(FJsonDataAssetPath::StaticStruct()) || It->IsChildOf(FSoftJsonDataAssetPtr::StaticStruct())
			|| It->IsChildOf(FJsonDataAssetPtr::StaticStruct()))
		{
			PropertyEditor.RegisterCustomPropertyTypeLayout(
				It->GetFName(),
				FOnGetPropertyTypeCustomizationInstance::CreateLambda([]() -> TSharedRef<IPropertyTypeCustomization> {
					return MakeShared<FJsonDataAssetPathCustomization>();
				}));
		}
	}
}

void UOUUEditorSubsystem::Deinitialize()
{
	auto pPropertyEditor = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor");

	if (pPropertyEditor)
	{
		for (TObjectIterator<UStruct> It; It; ++It)
		{
			if (It->IsChildOf(FJsonDataAssetPath::StaticStruct())
				|| It->IsChildOf(FSoftJsonDataAssetPtr::StaticStruct())
				|| It->IsChildOf(FJsonDataAssetPtr::StaticStruct()))
			{
				pPropertyEditor->UnregisterCustomPropertyTypeLayout(It->GetFName());
			}
		}
	}
}
