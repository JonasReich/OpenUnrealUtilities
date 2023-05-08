// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAssetEditor.h"

#include "AssetViewUtils.h"
#include "Editor.h"
#include "IAssetTools.h"
#include "IContentBrowserSingleton.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "JsonDataAsset/JsonDataAssetSubsystem.h"
#include "Logging/MessageLogMacros.h"
#include "PackageTools.h"
#include "Settings/EditorLoadingSavingSettings.h"

namespace OUU::Editor::JsonData
{
	void SyncContentBrowserToItems(const TArray<FString>& ItemPaths)
	{
		auto ContentBrowserSubsystem = GEditor->GetEditorSubsystem<UContentBrowserDataSubsystem>();
		TArray<FContentBrowserItem> Items;
		for (auto& ItemPath : ItemPaths)
		{
			Items.Add(ContentBrowserSubsystem->GetItemAtPath(*ItemPath, EContentBrowserItemTypeFilter::IncludeAll));
		}
		IContentBrowserSingleton::Get().SyncBrowserToItems(Items);
	}

	FJsonDataAssetPath ConvertMountedSourceFilenameToDataAssetPath(const FString& InFilename)
	{
		// Input paths have a format like "/Data/Plugins/OpenUnrealUtilities/NewDataAsset.json"
		FString PackagePath = InFilename;

		// Convert from a source (.json) path to a cache (.uasset) package path
		{
			PackagePath.ReplaceInline(TEXT(".json"), TEXT(""));
			PackagePath.ReplaceInline(
				*OUU::Runtime::JsonData::GetSourceMountPointRoot_Package(OUU::Runtime::JsonData::GameRootName),
				*OUU::Runtime::JsonData::GetCacheMountPointRoot_Package(OUU::Runtime::JsonData::GameRootName));
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
		auto* JsonAsset = Path.ForceReload();
		// If called on JsonAssets that do not have a source file anymore JsonAsset result may be null
		if (bPackageAlreadyExists && IsValid(JsonAsset))
		{
			FText ErrorMessage;
			UPackageTools::ReloadPackages(
				TArray<UPackage*>{JsonAsset->GetPackage()},
				OUT ErrorMessage,
				EReloadPackagesInteractionMode::Interactive);
		}
	}

	TAutoConsoleVariable<FString> CVar_Temp(TEXT("ouu.JsonData.SyncPath.Temp"), TEXT(""), TEXT(""));

	void ContentBrowser_NavigateToUAssets(const TArray<FJsonDataAssetPath>& Paths)
	{
		TArray<FString> PathStrings;
		for (auto Path : Paths)
		{
			auto PackagePath = Path.GetPackagePath();
			auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(PackagePath);
			FString ContentBrowserItemPath = FString::Printf(TEXT("/All%s.%s"), *PackagePath, *ObjectName);
			PathStrings.Add(ContentBrowserItemPath);
		}

		SyncContentBrowserToItems(PathStrings);
	}

	void ContentBrowser_NavigateToSources(const TArray<FJsonDataAssetPath>& Paths)
	{
		TArray<FString> PathStrings;
		for (auto Path : Paths)
		{
			auto PackagePath = Path.GetPackagePath();
			auto RootName = UJsonDataAssetSubsystem::Get().GetRootNameForPackagePath(PackagePath);

			PackagePath.ReplaceInline(
				*OUU::Runtime::JsonData::GetCacheMountPointRoot_Package(RootName),
				*OUU::Runtime::JsonData::GetSourceMountPointRoot_Package(RootName));
			FString ContentBrowserItemPath = FString::Printf(TEXT("/All%s.json"), *PackagePath);
			PathStrings.Add(ContentBrowserItemPath);
		}
		SyncContentBrowserToItems(PathStrings);
	}

	void ContentBrowser_OpenUnrealEditor(const FJsonDataAssetPath& Path)
	{
		auto* JsonObject = Path.LoadSynchronous();
		if (JsonObject)
		{
			AssetViewUtils::OpenEditorForAsset(JsonObject);
		}
		else
		{
			UE_MESSAGELOG(LoadErrors, Error, "Failed to load json data asset", Path.GetPackagePath());
		}
	}
	void ContentBrowser_OpenExternalEditor(const FJsonDataAssetPath& Path)
	{
		auto DiskPath = OUU::Runtime::JsonData::PackageToSourceFull(Path.GetPackagePath(), EJsonDataAccessMode::Read);
		FPlatformProcess::LaunchFileInDefaultExternalApplication(*DiskPath, nullptr, ELaunchVerb::Edit);
	}
} // namespace OUU::Editor::JsonData
