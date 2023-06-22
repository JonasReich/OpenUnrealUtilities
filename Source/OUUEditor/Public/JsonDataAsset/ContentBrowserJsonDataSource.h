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
	bool CanRenameItem(const FContentBrowserItemData& _Item, const FString* _NewName, FText* _OutErrorMsg) override;
	bool RenameItem(const FContentBrowserItemData& _Item, const FString& _NewName, FContentBrowserItemData& _OutNewItem)
		override;
	// ... moving
	bool CanMoveItem(const FContentBrowserItemData& _Item, const FName _DestPath, FText* _OutErrorMsg) override;
	bool MoveItem(const FContentBrowserItemData& _Item, const FName _DestPath) override;
	bool BulkMoveItems(TArrayView<const FContentBrowserItemData> _Items, const FName _DestPath) override;
	// ... copying
	bool CanCopyItem(const FContentBrowserItemData& _Item, const FName _DestPath, FText* _OutErrorMsg) override;
	bool CopyItem(const FContentBrowserItemData& _Item, const FName _DestPath) override;
	bool BulkCopyItems(TArrayView<const FContentBrowserItemData> _Items, const FName _DestPath) override;
	// ... duplicating
	bool CanDuplicateItem(const FContentBrowserItemData& _Item, FText* _OutErrorMsg) override;
	bool DuplicateItem(
		const FContentBrowserItemData& _Item,
		FContentBrowserItemDataTemporaryContext& _OutPendingItem) override;
	bool BulkDuplicateItems(
		TArrayView<const FContentBrowserItemData> _Items,
		TArray<FContentBrowserItemData>& _OutNewItems) override;

	// ... deleting
	bool CanDeleteItem(const FContentBrowserItemData& _Item, FText* _OutErrorMsg) override;
	bool DeleteItem(const FContentBrowserItemData& _Item) override;
	bool BulkDeleteItems(TArrayView<const FContentBrowserItemData> _Items) override;

	// ... legacy funcs to retrieve asset data and paths
	bool Legacy_TryGetPackagePath(const FContentBrowserItemData& _Item, FName& _OutPackagePath) override;
	bool Legacy_TryGetAssetData(const FContentBrowserItemData& _Item, FAssetData& _OutAssetData) override;

	// ... copy/paste asset reference
	bool AppendItemReference(const FContentBrowserItemData& _Item, FString& _OutStr) override;
	
	// ... thumbnail
	bool UpdateThumbnail(const FContentBrowserItemData& _Item, FAssetThumbnail& _OutThumbnail) override;
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
