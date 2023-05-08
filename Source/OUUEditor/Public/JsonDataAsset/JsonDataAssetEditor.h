// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "IAssetTools.h"
#include "JsonDataAsset/JsonDataAsset.h"

namespace OUU::Editor::JsonData
{
	void SyncContentBrowserToItem(FString ItemPath);

	FJsonDataAssetPath ConvertMountedSourceFilenameToDataAssetPath(const FString& InFilename);

	void PerformDiff(const FJsonDataAssetPath& Old, const FJsonDataAssetPath& New);
	void Reload(const FJsonDataAssetPath& Path);

	void ContentBrowser_NavigateToUAsset(const FJsonDataAssetPath& Path);
	void ContentBrowser_NavigateToSource(const FJsonDataAssetPath& Path);
	void ContentBrowser_OpenUnrealEditor(const FJsonDataAssetPath& Path);
	void ContentBrowser_OpenExternalEditor(const FJsonDataAssetPath& Path);
} // namespace OUU::Editor::JsonData
