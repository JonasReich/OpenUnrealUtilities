// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "ContentBrowserDelegates.h"
#include "ContentBrowserFileDataSource.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "Templates/SubclassOf.h"
#include "UObject/StrongObjectPtr.h"

#include "ContentBrowserJsonDataSource.generated.h"

class FContentBrowserFileItemDataPayload;
class FJsonFileSourceControlContextMenu;
class UContentBrowserFileDataSource;
class UContentBrowserDataMenuContext_FileMenu;
class UToolMenu;

USTRUCT()
struct FOUUJsonDataCreateParams
{
	GENERATED_BODY()
public:
	UPROPERTY()
	TSubclassOf<UJsonDataAsset> Class;
};

UCLASS()
class UContentBrowserJsonFileDataSource : public UContentBrowserFileDataSource
{
	GENERATED_BODY()
public:
	// - UContentBrowserDataSource

	// ... renaming
	bool CanRenameItem(const FContentBrowserItemData& InItem, const FString* InNewName, FText* OutErrorMsg) override;
	bool RenameItem(
		const FContentBrowserItemData& InItem,
		const FString& InNewName,
		FContentBrowserItemData& OutNewItem) override;
	// ... moving
	bool CanMoveItem(const FContentBrowserItemData& InItem, const FName InDestPath, FText* OutErrorMsg) override;
	bool MoveItem(const FContentBrowserItemData& InItem, const FName InDestPath) override;
	bool BulkMoveItems(TArrayView<const FContentBrowserItemData> InItems, const FName InDestPath) override;
	// ... deleting
	bool CanDeleteItem(const FContentBrowserItemData& InItem, FText* OutErrorMsg) override;
	bool DeleteItem(const FContentBrowserItemData& InItem) override;
	bool BulkDeleteItems(TArrayView<const FContentBrowserItemData> InItems) override;

	// ... legacy funcs to retrieve asset data and paths
	bool Legacy_TryGetPackagePath(const FContentBrowserItemData& InItem, FName& OutPackagePath) override;
	bool Legacy_TryGetAssetData(const FContentBrowserItemData& InItem, FAssetData& OutAssetData) override;

	// ... thumbnail
	bool UpdateThumbnail(const FContentBrowserItemData& InItem, FAssetThumbnail& OutThumbnail);
	// --
};

/**
 * Create once for the full json data integration.
 * The destructor takes care of all the teardown/cleanup.
 * It's expected to be used only from this editor module.
 */
struct FContentBrowserJsonDataSource
{
	FContentBrowserJsonDataSource();
	~FContentBrowserJsonDataSource();

	// To be used both for source files and also the generated uassets.
	void PopulateJsonFileContextMenu(UToolMenu* ContextMenu);

private:
	TStrongObjectPtr<UContentBrowserFileDataSource> JsonFileDataSource;
	TSharedPtr<FJsonFileSourceControlContextMenu> SourceControlContextMenu;

	TArray<TSharedRef<const FContentBrowserFileItemDataPayload>> GetSelectedFiles(
		const UContentBrowserDataMenuContext_FileMenu* ContextObject);

	TArray<FAssetIdentifier> GetContentBrowserSelectedJsonAssets(FOnContentBrowserGetSelection GetSelectionDelegate);

	bool ItemPassesFilter(
		const FName InFilePath,
		const FString& InFilename,
		const FContentBrowserDataFilter& InFilter,
		const bool bIsFile);

	bool GetItemAttribute(
		const FName InFilePath,
		const FString& InFilename,
		const bool InIncludeMetaData,
		const FName InAttributeKey,
		FContentBrowserItemDataAttributeValue& OutAttributeValue);

	bool CreateNewJsonAssetWithClass(const FString& PackagePath, UClass* AssetClass);
};
