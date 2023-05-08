// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "AssetTypeActions_Base.h"
#include "JsonDataAsset/JsonDataAsset.h"

class IToolkitHost;

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

	void OpenAssetEditor(const TArray<UObject*>& InObjects, TSharedPtr<IToolkitHost> EditWithinLevelEditor) override;
};
