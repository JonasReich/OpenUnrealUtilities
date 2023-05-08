// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

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