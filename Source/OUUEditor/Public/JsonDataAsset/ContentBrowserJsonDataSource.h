// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "UObject/StrongObjectPtr.h"

class FContentBrowserFileItemDataPayload;
class FJsonFileSourceControlContextMenu;
class UContentBrowserFileDataSource;
class UContentBrowserDataMenuContext_FileMenu;

class UToolMenu;

struct FJsonDataAssetPath;

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
