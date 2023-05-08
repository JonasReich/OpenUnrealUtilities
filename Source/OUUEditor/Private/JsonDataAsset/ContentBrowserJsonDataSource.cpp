// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/ContentBrowserJsonDataSource.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetDependencyGatherer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetViewUtils.h"
#include "ContentBrowserDataSource.h"
#include "ContentBrowserFileDataCore.h"
#include "ContentBrowserFileDataPayload.h"
#include "ContentBrowserFileDataSource.h"
#include "Editor.h"
#include "Engine.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameDelegates.h"
#include "HAL/PlatformFile.h"
#include "IContentBrowserDataModule.h"
#include "IContentBrowserSingleton.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "JsonDataAsset/JsonDataAssetEditor.h"
#include "JsonDataAsset/JsonFileSourceControlContextMenu.h"
#include "JsonObjectConverter.h"
#include "LogOpenUnrealUtilities.h"
#include "Logging/MessageLogMacros.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/JsonLibrary.h"
#include "Misc/Paths.h"
#include "Templates/ScopedAssign.h"
#include "ToolMenus.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/SavePackage.h"

#define JSON_CBROWSER_SOURCE_NAME TEXT("JsonData")

FContentBrowserJsonDataSource::FContentBrowserJsonDataSource()
{
	ContentBrowserFileData::FFileConfigData JsonFileConfig;

	SourceControlContextMenu = MakeShared<FJsonFileSourceControlContextMenu>();

	auto ItemPassesFilter = [](const FName InFilePath,
							   const FString& InFilename,
							   const FContentBrowserDataFilter& InFilter,
							   const bool bIsFile) {
		// Ignore files and folders with dots in them (besides the extension .)
		if (OUU::Runtime::JsonData::ShouldIgnoreInvalidExtensions())
		{
			// len(".json") -> 5
			FStringView FilenameWithoutSuffix(*InFilename, FMath::Min(InFilename.Len() - 5, 0));
			if (FilenameWithoutSuffix.Contains(TEXT(".")))
				return false;
		}

		const FContentBrowserDataPackageFilter* PackageFilter =
			InFilter.ExtraFilters.FindFilter<FContentBrowserDataPackageFilter>();
		if (PackageFilter && PackageFilter->PathPermissionList && PackageFilter->PathPermissionList->HasFiltering())
		{
			return PackageFilter->PathPermissionList
				->PassesStartsWithFilter(InFilePath, /*bAllowParentPaths*/ !bIsFile);
		}

		return true;
	};

	auto GetItemAttribute = [](const FName InFilePath,
							   const FString& InFilename,
							   const bool InIncludeMetaData,
							   const FName InAttributeKey,
							   FContentBrowserItemDataAttributeValue& OutAttributeValue) {
		if (InAttributeKey == ContentBrowserItemAttributes::ItemIsDeveloperContent)
		{
			const bool bIsDevelopersFolder = AssetViewUtils::IsDevelopersFolder(InFilePath.ToString());
			OutAttributeValue.SetValue(bIsDevelopersFolder);
			return true;
		}

		if (InAttributeKey == ContentBrowserItemAttributes::ItemIsLocalizedContent)
		{
			const bool bIsLocalizedFolder = FPackageName::IsLocalizedPackage(InFilePath.ToString());
			OutAttributeValue.SetValue(bIsLocalizedFolder);
			return true;
		}

		if (InAttributeKey == ContentBrowserItemAttributes::ItemIsEngineContent)
		{
			const bool bIsEngineFolder =
				AssetViewUtils::IsEngineFolder(InFilePath.ToString(), /*bIncludePlugins*/ true);
			OutAttributeValue.SetValue(bIsEngineFolder);
			return true;
		}

		if (InAttributeKey == ContentBrowserItemAttributes::ItemIsProjectContent)
		{
			const bool bIsProjectFolder =
				AssetViewUtils::IsProjectFolder(InFilePath.ToString(), /*bIncludePlugins*/ true);
			OutAttributeValue.SetValue(bIsProjectFolder);
			return true;
		}

		if (InAttributeKey == ContentBrowserItemAttributes::ItemIsPluginContent)
		{
			const bool bIsPluginFolder = AssetViewUtils::IsPluginFolder(InFilePath.ToString());
			OutAttributeValue.SetValue(bIsPluginFolder);
			return true;
		}

		return false;
	};

	ContentBrowserFileData::FDirectoryActions JsonDataDirectoryActions;
	JsonDataDirectoryActions.PassesFilter.BindLambda(ItemPassesFilter, false);
	JsonDataDirectoryActions.GetAttribute.BindLambda(GetItemAttribute);
	JsonFileConfig.SetDirectoryActions(JsonDataDirectoryActions);

	ContentBrowserFileData::FFileActions JsonFileActions;
	JsonFileActions.TypeExtension = TEXT("json");
	JsonFileActions.TypeName =
		FTopLevelAssetPath(TEXT("/Script/OUU.JsonData")); // Fake path to satisfy FFileActions requirements
	JsonFileActions.TypeDisplayName = INVTEXT("Json");
	JsonFileActions.TypeShortDescription = INVTEXT("Json file");
	JsonFileActions.TypeFullDescription = INVTEXT("A json data file");
	JsonFileActions.DefaultNewFileName = TEXT("new_json_asset");
	JsonFileActions.TypeColor = FColor(255, 156, 0);
	JsonFileActions.PassesFilter.BindLambda(ItemPassesFilter, true);
	JsonFileActions.GetAttribute.BindLambda(GetItemAttribute);

	// What is this?
	// JsonFileActions.Preview.BindLambda(ItemPreview);

	// #TODO Implement proper creation screen.
	// Disable creation for now. For the moment you have to create assets via the uasset "cache"
	JsonFileActions.CanCreate = false;

	JsonFileActions.Edit.BindLambda([this](const FName InFilePath, const FString& InFilename) -> bool {
		auto JsonPath = OUU::Editor::JsonData::ConvertMountedSourceFilenameToDataAssetPath(InFilePath.ToString());
		OUU::Editor::JsonData::ContentBrowser_OpenUnrealEditor(JsonPath);
		return true;
	});
	JsonFileConfig.RegisterFileActions(JsonFileActions);

	JsonFileDataSource.Reset(
		NewObject<UContentBrowserFileDataSource>(GetTransientPackage(), JSON_CBROWSER_SOURCE_NAME));
	JsonFileDataSource->Initialize(JsonFileConfig);

	TArray<FString> RootPaths;
	FPackageName::QueryRootContentPaths(RootPaths);
	const FString JsonDir = OUU::Runtime::JsonData::GetSourceRoot_Full(EJsonDataAccessMode::Read);

	// Add a file mount for the data source.
	{
		// At the moment we only support a single external folder, but this would also allow folders nested inside of
		// content roots (like it's done for the Python extension)

		// #TODO use GetSourceMountPointRoot_Package and GetSourceMountPointRoot_DiskFull instead!

		auto FileMountPath = OUU::Runtime::JsonData::GetSourceRoot_ProjectRelative(EJsonDataAccessMode::Read);
		auto FileMountDiskPath = OUU::Runtime::JsonData::GetSourceRoot_Full(EJsonDataAccessMode::Read);
		// Both of these paths have a trailing slash, but AddFileMount expects no trailing slash
		FileMountPath.RemoveFromEnd(TEXT("/"));
		FileMountDiskPath.RemoveFromEnd(TEXT("/"));
		// Add a leading slash, so it's a path root
		FileMountPath.InsertAt(0, TEXT("/"));

		JsonFileDataSource->AddFileMount(*FileMountPath, *FileMountDiskPath);
	}

	FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda([this]() {
		UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
		if (ensure(IsValid(ContentBrowserData)))
		{
			ContentBrowserData->ActivateDataSource(JSON_CBROWSER_SOURCE_NAME);
		}
	});

	{
		FToolMenuOwnerScoped OwnerScoped(this);
		if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.ItemContextMenu.JsonData"))
		{
			Menu->AddDynamicSection(
				TEXT("DynamicSection_JsonData"),
				FNewToolMenuDelegate::CreateRaw(this, &FContentBrowserJsonDataSource::PopulateJsonFileContextMenu));
		}
	}
}

FContentBrowserJsonDataSource::~FContentBrowserJsonDataSource()
{
	if (JsonFileDataSource.IsValid())
	{
		auto* FileDataSource = JsonFileDataSource.Get();
		if (IsValid(FileDataSource))
		{
			// #TODO Fix access violation here.
			// For now we just let this leak, because we want to keep it until shutdown and do not support reloading.
			// FileDataSource->Shutdown();
		}
	}
}

void FContentBrowserJsonDataSource::PopulateJsonFileContextMenu(UToolMenu* ContextMenu)
{
	const UContentBrowserDataMenuContext_FileMenu* ContextObject =
		ContextMenu->FindContext<UContentBrowserDataMenuContext_FileMenu>();
	checkf(ContextObject, TEXT("Required context UContentBrowserDataMenuContext_FileMenu was missing!"));

	if (JsonFileDataSource.IsValid() == false)
	{
		return;
	}

	auto SelectedJsonFiles = GetSelectedFiles(ContextObject);

	// Only add the file items if we have at least one file path selected
	if (SelectedJsonFiles.Num() > 0)
	{
		SourceControlContextMenu->MakeContextMenu(ContextMenu, SelectedJsonFiles);

		// Custom Action
		{
			FToolMenuSection& Section = ContextMenu->AddSection("JsonData_CustomActions", INVTEXT("Json Data"));
			Section.InsertPosition.Position = EToolMenuInsertType::First;

			const FExecuteAction OpenExternalAction = FExecuteAction::CreateLambda([this, SelectedJsonFiles]() {
				for (const TSharedRef<const FContentBrowserFileItemDataPayload>& SelectedJsonFile : SelectedJsonFiles)
				{
					FPlatformProcess::LaunchFileInDefaultExternalApplication(
						*SelectedJsonFile->GetFilename(),
						nullptr,
						ELaunchVerb::Edit);
				}
			});

			Section.AddMenuEntry(
				"OpenInDefaultExternalApplication",
				INVTEXT("Edit json..."),
				INVTEXT("Edit the json text file in the default external application"),
				FSlateIcon(),
				FUIAction(OpenExternalAction));

			const FExecuteAction ReloadAction = FExecuteAction::CreateLambda([this, SelectedJsonFiles]() {
				for (const TSharedRef<const FContentBrowserFileItemDataPayload>& SelectedJsonFile : SelectedJsonFiles)
				{
					auto JsonPath = FJsonDataAssetPath::FromPackagePath(OUU::Runtime::JsonData::SourceFullToPackage(
						SelectedJsonFile->GetFilename(),
						EJsonDataAccessMode::Read));

					OUU::Editor::JsonData::Reload(JsonPath);
				}
			});

			Section.AddMenuEntry(
				"ReloadJson",
				INVTEXT("Reload"),
				INVTEXT("Reload the data from the json file"),
				FSlateIcon(),
				FUIAction(ReloadAction));

			const FExecuteAction NavigateToUAssetAction = FExecuteAction::CreateLambda([this, SelectedJsonFiles]() {
				if (SelectedJsonFiles.Num() > 0)
				{
					const TSharedRef<const FContentBrowserFileItemDataPayload>& SelectedJsonFile = SelectedJsonFiles[0];
					auto JsonPath = FJsonDataAssetPath::FromPackagePath(OUU::Runtime::JsonData::SourceFullToPackage(
						SelectedJsonFile->GetFilename(),
						EJsonDataAccessMode::Read));

					OUU::Editor::JsonData::ContentBrowser_NavigateToUAsset(JsonPath);
				}
			});

			Section.AddMenuEntry(
				"NavigateToUAsset",
				INVTEXT("Show generated asset"),
				INVTEXT("Navigate to the generated uasset file in the content browser."),
				FSlateIcon(),
				FUIAction(NavigateToUAssetAction));
		}
	}
}

TArray<TSharedRef<const FContentBrowserFileItemDataPayload>> FContentBrowserJsonDataSource::GetSelectedFiles(
	const UContentBrowserDataMenuContext_FileMenu* ContextObject)
{
	// Extract the internal file paths that belong to this data source from the full list of selected paths given in the
	// context

	TArray<TSharedRef<const FContentBrowserFileItemDataPayload>> SelectedJsonFiles;
	for (const FContentBrowserItem& SelectedItem : ContextObject->SelectedItems)
	{
		if (const FContentBrowserItemData* ItemDataPtr = SelectedItem.GetPrimaryInternalItem())
		{
			if (TSharedPtr<const FContentBrowserFileItemDataPayload> FilePayload =
					ContentBrowserFileData::GetFileItemPayload(JsonFileDataSource.Get(), *ItemDataPtr))
			{
				SelectedJsonFiles.Add(FilePayload.ToSharedRef());
			}
		}
	}
	return SelectedJsonFiles;
}

#undef JSON_CBROWSER_SOURCE_NAME
