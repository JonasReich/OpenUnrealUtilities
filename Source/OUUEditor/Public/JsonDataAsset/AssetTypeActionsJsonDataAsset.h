// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "AssetTypeActions_Base.h"
#include "JsonDataAsset/JsonDataAsset.h"

class FAssetTypeActions_JsonDataAsset : public FAssetTypeActions_Base
{
public:
	// - IAssetTypeActions
	FText GetName() const override
	{
		return NSLOCTEXT("AssetTypeActions", "AssetTypeActions_JsonDataAsset", "Json Data Asset");
	}
	FColor GetTypeColor() const override { return FColor(190, 247, 120); }
	UClass* GetSupportedClass() const override { return UJsonDataAsset::StaticClass(); }
	uint32 GetCategories() override { return EAssetTypeCategories::None; }
	void GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section) override;
	// --

	// #TODO Figure out if we can implement simple import actions
	// (something probably has to implementFReimportHandler?)
	/*
	bool IsImportedAsset() const { return true; }
	void GetResolvedSourceFilePaths(const TArray<UObject*>& TypeAssets, TArray<FString>& OutSourceFilePaths)
		const override
	{
		for (auto& Asset : TypeAssets)
		{
			const auto JsonDataAsset = CastChecked<UJsonDataAsset>(Asset);
			OutSourceFilePaths.Add(JsonDataAsset->GetJsonFilePathAbs(EJsonDataAccessMode::Read));
		}
	}
	*/

	// #TODO implement custom asset editor that has buttons to link back to json file
	/*
	void OpenAssetEditor(
		const TArray<UObject*>& InObjects,
		TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;
	*/
};
