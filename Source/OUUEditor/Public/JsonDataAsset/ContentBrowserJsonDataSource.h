// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

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
	virtual bool CanMoveItem(const FContentBrowserItemData& InItem, const FName InDestPath, FText* OutErrorMsg)
		override;
	virtual bool MoveItem(const FContentBrowserItemData& InItem, const FName InDestPath) override;
	virtual bool BulkMoveItems(TArrayView<const FContentBrowserItemData> InItems, const FName InDestPath) override;
	// --
};

/**
 * Create once for the full json data integration.
 * The destructor takes care of all the teardown/cleanup.
 * It's expected to be used only from this editor module.
 * // #TODO-OUU Move to the Private/ folder
 */
struct FContentBrowserJsonDataSource
{
	FContentBrowserJsonDataSource();
	~FContentBrowserJsonDataSource();

private:
	TStrongObjectPtr<UContentBrowserFileDataSource> JsonFileDataSource;
	TSharedPtr<FJsonFileSourceControlContextMenu> SourceControlContextMenu;

	void PopulateJsonFileContextMenu(UToolMenu* ContextMenu);

	TArray<TSharedRef<const FContentBrowserFileItemDataPayload>> GetSelectedFiles(
		const UContentBrowserDataMenuContext_FileMenu* ContextObject);
};
