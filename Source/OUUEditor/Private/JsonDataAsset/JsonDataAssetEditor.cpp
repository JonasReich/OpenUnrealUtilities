// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAssetEditor.h"

#include "AssetViewUtils.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "PackageTools.h"
#include "Editor.h"
#include "Settings/EditorLoadingSavingSettings.h"

namespace OUU::Editor::JsonData
{
	void SyncContentBrowserToItem(FString ItemPath)
	{
		auto Item = GEditor->GetEditorSubsystem<UContentBrowserDataSubsystem>()
						->GetItemAtPath(*ItemPath, EContentBrowserItemTypeFilter::IncludeAll);
		IContentBrowserSingleton::Get().SyncBrowserToItems({Item});
	}

	FJsonDataAssetPath ConvertMountedSourceFilenameToDataAssetPath(const FString& InFilename)
	{
		FString PackagePath = InFilename;

		// Convert from a source (.json) path to a cache (.uasset) package path
		{
			PackagePath.ReplaceInline(TEXT(".json"), TEXT(""));
			PackagePath.ReplaceInline(
				*OUU::Runtime::JsonData::GetSourceMountPointRoot_Package(),
				*OUU::Runtime::JsonData::GetCacheMountPointRoot_Package());
		}

		FJsonDataAssetPath JsonPath;
		JsonPath.SetPackagePath(PackagePath);
		return JsonPath;
	}

	void PerformDiff(const FJsonDataAssetPath& Old, const FJsonDataAssetPath& New)
	{
		FString OldTextFilename =
			OUU::Runtime::JsonData::PackageToSourceFull(Old.GetPackagePath(), EJsonDataAccessMode::Read);
		FString NewTextFilename =
			OUU::Runtime::JsonData::PackageToSourceFull(New.GetPackagePath(), EJsonDataAccessMode::Read);
		FString DiffCommand = GetDefault<UEditorLoadingSavingSettings>()->TextDiffToolPath.FilePath;

		IAssetTools::Get().CreateDiffProcess(DiffCommand, OldTextFilename, NewTextFilename);
	}

	void Reload(const FJsonDataAssetPath& Path)
	{
		auto PackagePath = Path.GetPackagePath();
		FString PackageFilename;
		const bool bPackageAlreadyExists = FPackageName::DoesPackageExist(PackagePath, &PackageFilename);
		auto* JsonAsset = Path.Resolve();

		if (bPackageAlreadyExists)
		{
			FText ErrorMessage;
			UPackageTools::ReloadPackages(
				TArray<UPackage*>{JsonAsset->GetPackage()},
				OUT ErrorMessage,
				EReloadPackagesInteractionMode::Interactive);
		}
	}

	TAutoConsoleVariable<FString> CVar_Temp(TEXT("ouu.JsonData.SyncPath.Temp"), TEXT(""), TEXT(""));

	void ContentBrowser_NavigateToUAsset(const FJsonDataAssetPath& Path)
	{
		auto PackagePath = Path.GetPackagePath();
		auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(PackagePath);
		FString ContentBrowserItemPath = FString::Printf(TEXT("/All%s.%s"), *PackagePath, *ObjectName);
		SyncContentBrowserToItem(ContentBrowserItemPath);
	}

	void ContentBrowser_NavigateToSource(const FJsonDataAssetPath& Path)
	{
		auto PackagePath = Path.GetPackagePath();
		PackagePath.ReplaceInline(
			*OUU::Runtime::JsonData::GetCacheMountPointRoot_Package(),
			*OUU::Runtime::JsonData::GetSourceMountPointRoot_Package());
		SyncContentBrowserToItem(FString::Printf(TEXT("/All%s.json"), *PackagePath));
	}

	void ContentBrowser_OpenUnrealEditor(const FJsonDataAssetPath& Path)
	{
		auto* JsonObject = Path.Resolve();
		if (JsonObject)
		{
			AssetViewUtils::OpenEditorForAsset(JsonObject);
		}
	}
	void ContentBrowser_OpenExternalEditor(const FJsonDataAssetPath& Path)
	{
		auto DiskPath = OUU::Runtime::JsonData::PackageToSourceFull(Path.GetPackagePath(), EJsonDataAccessMode::Read);
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*DiskPath, nullptr, ELaunchVerb::Edit);
	}
} // namespace OUU::Editor::JsonData
