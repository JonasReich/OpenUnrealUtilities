// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "IAssetTools.h"
#include "JsonDataAsset/JsonDataAsset.h"

namespace OUU::Editor::JsonData
{
	void SyncContentBrowserToItems(const TArray<FString>& ItemPaths);

	FJsonDataAssetPath ConvertMountedSourceFilenameToDataAssetPath(const FString& InFilename);

	void PerformDiff(const FJsonDataAssetPath& Old, const FJsonDataAssetPath& New);
	void Reload(const FJsonDataAssetPath& Path);

	void ContentBrowser_NavigateToUAssets(const TArray<FJsonDataAssetPath>& Paths);
	void ContentBrowser_NavigateToSources(const TArray<FJsonDataAssetPath>& Paths);
	void ContentBrowser_OpenUnrealEditor(const FJsonDataAssetPath& Path);
	void ContentBrowser_OpenExternalEditor(const FJsonDataAssetPath& Path);
} // namespace OUU::Editor::JsonData
