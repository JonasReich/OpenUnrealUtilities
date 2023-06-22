// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/ContentBrowserJsonDataSource.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetDependencyGatherer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetViewUtils.h"
#include "ContentBrowserDataSource.h"
#include "ContentBrowserFileDataCore.h"
#include "ContentBrowserFileDataPayload.h"
#include "Editor.h"
#include "Engine.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "GameDelegates.h"
#include "HAL/PlatformFile.h"
#include "IContentBrowserDataModule.h"
#include "JsonDataAsset/JsonDataAssetEditor.h"
#include "JsonDataAsset/JsonDataAssetGlobals.h"
#include "JsonDataAsset/JsonDataAssetSubsystem.h"
#include "JsonDataAsset/JsonFileSourceControlContextMenu.h"
#include "LogOpenUnrealUtilities.h"
#include "Logging/MessageLogMacros.h"
#include "Misc/JsonLibrary.h"
#include "Misc/Paths.h"
#include "Slate/SimpleClassPickerDialog.h"
#include "ToolMenus.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/SavePackage.h"

namespace OUU::Editor::Private
{
	const FName JsonContentBrowserSourceName = TEXT("JsonData");
}

bool UContentBrowserJsonFileDataSource::CanRenameItem(
	const FContentBrowserItemData& InItem,
	const FString* InNewName,
	FText* OutErrorMsg)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		// Assume this case is for asset creation
		return true;
	}

	return ContentBrowserItem.CanRename(InNewName, OUT OutErrorMsg);
}

bool UContentBrowserJsonFileDataSource::RenameItem(
	const FContentBrowserItemData& InItem,
	const FString& InNewName,
	FContentBrowserItemData& OutNewItem)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	return ContentBrowserItem.Rename(InNewName);
}

bool UContentBrowserJsonFileDataSource::CanMoveItem(
	const FContentBrowserItemData& InItem,
	const FName InDestPath,
	FText* OutErrorMsg)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		if (OutErrorMsg)
		{
			*OutErrorMsg = INVTEXT("Can't move json source without generated asset.");
		}
		return false;
	}

	auto GeneratedDestPath =
		OUU::Editor::JsonData::ConvertMountedSourceFilenameToMountedDataAssetFilename(InDestPath.ToString());

	return ContentBrowserItem.CanMove(*GeneratedDestPath, OUT OutErrorMsg);
}

bool UContentBrowserJsonFileDataSource::MoveItem(const FContentBrowserItemData& InItem, const FName InDestPath)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	auto GeneratedDestPath =
		OUU::Editor::JsonData::ConvertMountedSourceFilenameToMountedDataAssetFilename(InDestPath.ToString());

	bool bMoved = ContentBrowserItem.Move(*GeneratedDestPath);

	if (ensure(bMoved) && InItem.IsFile())
	{
		auto DestJsonAssetPath =
			OUU::Editor::JsonData::ConvertMountedSourceFilenameToDataAssetPath(InDestPath.ToString());
		ensure(IsValid(DestJsonAssetPath.ResolveObject()));
	}

	return bMoved;
}

bool UContentBrowserJsonFileDataSource::BulkMoveItems(
	TArrayView<const FContentBrowserItemData> InItems,
	const FName InDestPath)
{
	bool bAllMoved = true;
	for (auto& Item : InItems)
	{
		auto ContentBrowserItem =
			OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(Item.GetInternalPath().ToString());
		if (ContentBrowserItem.IsValid() == false)
		{
			return false;
		}

		auto GeneratedDestPath =
			OUU::Editor::JsonData::ConvertMountedSourceFilenameToMountedDataAssetFilename(InDestPath.ToString());

		bool bMoved = ContentBrowserItem.Move(*GeneratedDestPath);
		ensure(bMoved);
		bAllMoved &= bMoved;
	}
	return bAllMoved;
}

bool UContentBrowserJsonFileDataSource::CanCopyItem(
	const FContentBrowserItemData& _Item,
	const FName _DestPath,
	FText* _OutErrorMsg)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(_Item.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		if (_OutErrorMsg)
		{
			*_OutErrorMsg = INVTEXT("Can't copy json source without generated asset.");
		}
		return false;
	}

	auto GeneratedDestPath =
		OUU::Editor::JsonData::ConvertMountedSourceFilenameToMountedDataAssetFilename(_DestPath.ToString());

	return ContentBrowserItem.CanCopy(*GeneratedDestPath, OUT _OutErrorMsg);
}

bool UContentBrowserJsonFileDataSource::CopyItem(const FContentBrowserItemData& _Item, const FName _DestPath)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(_Item.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	auto GeneratedDestPath =
		OUU::Editor::JsonData::ConvertMountedSourceFilenameToMountedDataAssetFilename(_DestPath.ToString());

	return ContentBrowserItem.Copy(*GeneratedDestPath);
}

bool UContentBrowserJsonFileDataSource::BulkCopyItems(
	TArrayView<const FContentBrowserItemData> _Items,
	const FName _DestPath)
{
	bool bAllCopied = true;
	for (auto& Item : _Items)
	{
		auto ContentBrowserItem =
			OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(Item.GetInternalPath().ToString());
		if (ContentBrowserItem.IsValid() == false)
		{
			return false;
		}

		auto GeneratedDestPath =
			OUU::Editor::JsonData::ConvertMountedSourceFilenameToMountedDataAssetFilename(_DestPath.ToString());

		bool bCopied = ContentBrowserItem.Copy(*GeneratedDestPath);
		ensure(bCopied);
		bAllCopied &= bCopied;
	}
	return bAllCopied;
}

bool UContentBrowserJsonFileDataSource::CanDuplicateItem(const FContentBrowserItemData& _Item, FText* _OutErrorMsg)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(_Item.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		if (_OutErrorMsg)
		{
			*_OutErrorMsg = INVTEXT("Can't duplicate json source without generated asset.");
		}
		return false;
	}

	return ContentBrowserItem.CanDuplicate(OUT _OutErrorMsg);
}

bool UContentBrowserJsonFileDataSource::DuplicateItem(
	const FContentBrowserItemData& _Item,
	FContentBrowserItemDataTemporaryContext& _OutPendingItem)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(_Item.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	auto Result = ContentBrowserItem.Duplicate();
	return Result.IsValid();
}

bool UContentBrowserJsonFileDataSource::BulkDuplicateItems(
	TArrayView<const FContentBrowserItemData> _Items,
	TArray<FContentBrowserItemData>& _OutNewItems)
{
	bool bAllDuplicated = true;
	for (auto& Item : _Items)
	{
		auto ContentBrowserItem =
			OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(Item.GetInternalPath().ToString());
		if (ContentBrowserItem.IsValid() == false)
		{
			return false;
		}

		auto DuplicationContext = ContentBrowserItem.Duplicate();
		bAllDuplicated &= DuplicationContext.IsValid();
	}
	return bAllDuplicated;
}

bool UContentBrowserJsonFileDataSource::CanDeleteItem(const FContentBrowserItemData& InItem, FText* OutErrorMsg)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		if (OutErrorMsg)
		{
			*OutErrorMsg = INVTEXT("Can't delete json source without generated asset.");
		}
		return false;
	}

	return ContentBrowserItem.CanDelete(OUT OutErrorMsg);
}

bool UContentBrowserJsonFileDataSource::DeleteItem(const FContentBrowserItemData& InItem)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	return ContentBrowserItem.Delete();
}

bool UContentBrowserJsonFileDataSource::BulkDeleteItems(TArrayView<const FContentBrowserItemData> InItems)
{
	bool bAllDeleted = true;
	for (auto& Item : InItems)
	{
		auto ContentBrowserItem =
			OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(Item.GetInternalPath().ToString());
		if (ContentBrowserItem.IsValid() == false)
		{
			return false;
		}

		bool bDeleted = ContentBrowserItem.Delete();
		// Deletion can be cancelled via dialog, so it's okay if it's false
		// ensure(bDeleted);
		bAllDeleted &= bDeleted;
	}
	return bAllDeleted;
}

bool UContentBrowserJsonFileDataSource::Legacy_TryGetPackagePath(
	const FContentBrowserItemData& InItem,
	FName& OutPackagePath)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	return ContentBrowserItem.Legacy_TryGetPackagePath(OUT OutPackagePath);
}

bool UContentBrowserJsonFileDataSource::Legacy_TryGetAssetData(
	const FContentBrowserItemData& InItem,
	FAssetData& OutAssetData)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	return ContentBrowserItem.Legacy_TryGetAssetData(OUT OutAssetData);
}

bool UContentBrowserJsonFileDataSource::UpdateThumbnail(
	const FContentBrowserItemData& InItem,
	FAssetThumbnail& OutThumbnail)
{
	auto ContentBrowserItem =
		OUU::Editor::JsonData::GetGeneratedAssetContentBrowserItem(InItem.GetInternalPath().ToString());
	if (ContentBrowserItem.IsValid() == false)
	{
		return false;
	}

	return ContentBrowserItem.UpdateThumbnail(OUT OutThumbnail);
}

FContentBrowserJsonDataSource::FContentBrowserJsonDataSource()
{
	ContentBrowserFileData::FFileConfigData JsonFileConfig;

	SourceControlContextMenu = MakeShared<FJsonFileSourceControlContextMenu>();

	ContentBrowserFileData::FDirectoryActions JsonDataDirectoryActions;
	JsonDataDirectoryActions.PassesFilter.BindRaw(this, &FContentBrowserJsonDataSource::ItemPassesFilter, false);
	JsonDataDirectoryActions.GetAttribute.BindRaw(this, &FContentBrowserJsonDataSource::GetItemAttribute);
	JsonFileConfig.SetDirectoryActions(JsonDataDirectoryActions);

	ContentBrowserFileData::FFileActions JsonFileActions;
	JsonFileActions.TypeExtension = TEXT("json");
	JsonFileActions.TypeName =
		FTopLevelAssetPath(TEXT("/Script/OUU.JsonData")); // Fake path to satisfy FFileActions requirements
	JsonFileActions.TypeDisplayName = INVTEXT("Json Source File");
	JsonFileActions.TypeShortDescription = INVTEXT("Json file");
	JsonFileActions.TypeFullDescription = INVTEXT("A json data file");
	JsonFileActions.DefaultNewFileName = TEXT("new_json_asset");
	JsonFileActions.TypeColor = FColor(255, 156, 0);
	JsonFileActions.PassesFilter.BindRaw(this, &FContentBrowserJsonDataSource::ItemPassesFilter, true);
	JsonFileActions.GetAttribute.BindRaw(this, &FContentBrowserJsonDataSource::GetItemAttribute);

	// What is this?
	// JsonFileActions.Preview.BindLambda(ItemPreview);

	// For now allow creation in all directories (default if not assigned)
	JsonFileActions.CanCreate.BindLambda(
		[](const FName InDestFolderPath, const FString& InDestFolder, FText* OutErrorMsg) -> bool {
			auto PluginsRoot =
				OUU::Runtime::JsonData::GetSourceMountPointRoot_Package(OUU::Runtime::JsonData::GameRootName)
				+ "Plugins";

			if (PluginsRoot.Equals(*InDestFolderPath.ToString(), ESearchCase::IgnoreCase))
			{
				if (OutErrorMsg)
				{
					*OutErrorMsg = FText::FromString(
						FString::Printf(TEXT("Can't create json assets in %s root folder"), *PluginsRoot));
				}
				return false;
			}

			return true;
		});
	JsonFileActions.ConfigureCreation.BindLambda(
		[this](FString& OutFileBasename, FStructOnScope& OutCreationConfig) -> bool {
			OutCreationConfig.Initialize(FOUUJsonDataCreateParams::StaticStruct());
			auto& Config = *(FOUUJsonDataCreateParams*)OutCreationConfig.GetStructMemory();

			EClassFlags DisallowedClassFlags =
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;

			Config.Class = OUU::Editor::OpenSimpleClassPickerDialog(
				UJsonDataAsset::StaticClass(),
				DisallowedClassFlags,
				INVTEXT("Pick a Json Data Asset Class"));

			return Config.Class != nullptr;
		});

	JsonFileActions.Create.BindLambda(
		[this](const FName InFilePath, const FString& InFilenameWrong, const FStructOnScope& CreationConfig) -> bool {
			auto& Config = *(FOUUJsonDataCreateParams*)CreationConfig.GetStructMemory();
			UClass* Class = Config.Class.Get();
			if (IsValid(Class) == false)
			{
				UE_LOG(
					LogOpenUnrealUtilities,
					Warning,
					TEXT("Failed to create new json data asset %s (no class selected)"),
					*InFilenameWrong);
				return false;
			}

			// InFilenameWrong is wrong for plugin mounted files. It's correct after converting back and forth though.
			auto PackagePath = OUU::Runtime::JsonData::SourceFullToPackage(InFilenameWrong, EJsonDataAccessMode::Read);
			return CreateNewJsonAssetWithClass(PackagePath, Class);
		});

	JsonFileActions.Edit.BindLambda([this](const FName InFilePath, const FString& InFilename) -> bool {
		auto JsonPath = OUU::Editor::JsonData::ConvertMountedSourceFilenameToDataAssetPath(InFilePath.ToString());
		OUU::Editor::JsonData::ContentBrowser_OpenUnrealEditor(JsonPath);
		return true;
	});

	JsonFileConfig.RegisterFileActions(JsonFileActions);

	JsonFileDataSource.Reset(NewObject<UContentBrowserJsonFileDataSource>(
		GetTransientPackage(),
		OUU::Editor::Private::JsonContentBrowserSourceName));
	JsonFileDataSource->Initialize(JsonFileConfig);

	// Add file mounts for the data source.
	{
		FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda([this]() {
			auto AddMountLambda = [this](const FName& RootName) {
				auto FileMountPath = OUU::Runtime::JsonData::GetSourceMountPointRoot_Package(RootName);
				auto FileMountDiskPath = OUU::Runtime::JsonData::GetSourceMountPointRoot_DiskFull(RootName);
				// Both of these paths have a trailing slash, but AddFileMount expects no trailing slash
				FileMountPath.RemoveFromEnd(TEXT("/"));
				FileMountDiskPath.RemoveFromEnd(TEXT("/"));

				JsonFileDataSource->AddFileMount(*FileMountPath, *FileMountDiskPath);
			};

			// Always add a mount point for the game data folder
			AddMountLambda(OUU::Runtime::JsonData::GameRootName);

			// Also add mount points for all plugins that are already registered...
			for (auto& PluginRootName : UJsonDataAssetSubsystem::Get().GetAllPluginRootNames())
			{
				AddMountLambda(PluginRootName);
			}

			// ...and react to new plugin being registered after this file source was set-up.
			UJsonDataAssetSubsystem::Get().OnNewPluginRootAdded.AddLambda(AddMountLambda);
		});
	}

	FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda([this]() {
		UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
		if (ensure(IsValid(ContentBrowserData)))
		{
			ContentBrowserData->ActivateDataSource(OUU::Editor::Private::JsonContentBrowserSourceName);
		}
	});

	{
		// This is automatically adds the additional options to the generated assets.
		// Because the data source returns the package paths of the generated packages, we also get all these options on
		// the source files. At the end of the day, as long as they work with both kind of content browser entries,
		// we're fine.
		// Before returning the generated package paths from the data source, we also needed to register these context
		// menu entries for "ContentBrowser.ItemContextMenu.JsonData"
		FToolMenuOwnerScoped OwnerScoped(this);
		if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.JsonDataAsset"))
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
		// "Imported Asset" Actions
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
				INVTEXT("Edit .json Source"),
				INVTEXT("Edit the json text file in the default external application (text editor)"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Edit"),
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
				INVTEXT("Reload all property data from the json file"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Refresh"),
				FUIAction(ReloadAction));

			const FExecuteAction NavigateToUAssetAction = FExecuteAction::CreateLambda([this, SelectedJsonFiles]() {
				if (SelectedJsonFiles.Num() > 0)
				{
					TArray<FJsonDataAssetPath> Paths;
					for (auto& SelectedJsonFile : SelectedJsonFiles)
					{
						auto JsonPath = FJsonDataAssetPath::FromPackagePath(OUU::Runtime::JsonData::SourceFullToPackage(
							SelectedJsonFile->GetFilename(),
							EJsonDataAccessMode::Read));
						Paths.Add(JsonPath);
					}

					OUU::Editor::JsonData::ContentBrowser_NavigateToUAssets(Paths);
				}
			});

			Section.AddMenuEntry(
				"BrowseToGeneratedAsset",
				INVTEXT("Browse to Generated Asset"),
				INVTEXT("Browses to the generated asset file and selects it in the most recently used Content Browser"),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "SystemWideCommands.FindInContentBrowser"),
				FUIAction(NavigateToUAssetAction));
		}

		SourceControlContextMenu->MakeContextMenu(ContextMenu, SelectedJsonFiles);
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

TArray<FAssetIdentifier> FContentBrowserJsonDataSource::GetContentBrowserSelectedJsonAssets(
	FOnContentBrowserGetSelection GetSelectionDelegate)
{
	TArray<FAssetIdentifier> OutAssetPackages;
	TArray<FAssetData> SelectedAssets;
	TArray<FString> SelectedPaths;

	if (GetSelectionDelegate.IsBound())
	{
		TArray<FString> SelectedVirtualPaths;
		GetSelectionDelegate.Execute(SelectedAssets, SelectedVirtualPaths);

		for (const FString& VirtualPath : SelectedVirtualPaths)
		{
			FString InvariantPath;
			if (IContentBrowserDataModule::Get().GetSubsystem()->TryConvertVirtualPath(VirtualPath, InvariantPath)
				== EContentBrowserPathType::Internal)
			{
				SelectedPaths.Add(InvariantPath);
			}
		}
	}

	return OutAssetPackages;
}

bool FContentBrowserJsonDataSource::ItemPassesFilter(
	const FName InFilePath,
	const FString& InFilename,
	const FContentBrowserDataFilter& InFilter,
	const bool bIsFile)
{
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
		return PackageFilter->PathPermissionList->PassesStartsWithFilter(InFilePath, /*bAllowParentPaths*/ !bIsFile);
	}

	return true;
}

bool FContentBrowserJsonDataSource::GetItemAttribute(
	const FName InFilePath,
	const FString& InFilename,
	const bool InIncludeMetaData,
	const FName InAttributeKey,
	FContentBrowserItemDataAttributeValue& OutAttributeValue)
{
	if (InAttributeKey == ContentBrowserItemAttributes::ItemIsDeveloperContent)
	{
		const bool bIsDevelopersFolder = AssetViewUtils::IsDevelopersFolder(InFilePath.ToString());
		OutAttributeValue.SetValue(bIsDevelopersFolder);
		return true;
	}

	if (InAttributeKey == ContentBrowserItemAttributes::ItemIsLocalizedContent)
	{
		// Json data assets can't be localized
		const bool bIsLocalizedFolder = false; // FPackageName::IsLocalizedPackage(InFilePath.ToString());
		OutAttributeValue.SetValue(bIsLocalizedFolder);
		return true;
	}

	if (InAttributeKey == ContentBrowserItemAttributes::ItemIsEngineContent)
	{
		// Json data assets can't never be used on engine level.
		const bool bIsEngineFolder = false;
		OutAttributeValue.SetValue(bIsEngineFolder);
		return true;
	}

	if (InAttributeKey == ContentBrowserItemAttributes::ItemIsProjectContent)
	{
		auto RootName = UJsonDataAssetSubsystem::Get().GetRootNameForSourcePath(InFilePath.ToString());
		const bool bIsProjectFolder = RootName == OUU::Runtime::JsonData::GameRootName;
		OutAttributeValue.SetValue(bIsProjectFolder);
		return true;
	}

	if (InAttributeKey == ContentBrowserItemAttributes::ItemIsPluginContent)
	{
		auto RootName = UJsonDataAssetSubsystem::Get().GetRootNameForSourcePath(InFilePath.ToString());
		// All files in other roots are guaranteed from plugins.
		const bool bIsPluginFolder = RootName != OUU::Runtime::JsonData::GameRootName;
		OutAttributeValue.SetValue(bIsPluginFolder);
		return true;
	}

	return false;
};

bool FContentBrowserJsonDataSource::CreateNewJsonAssetWithClass(const FString& PackagePath, UClass* AssetClass)
{
	auto FileNameCorrected = OUU::Runtime::JsonData::PackageToSourceFull(PackagePath, EJsonDataAccessMode::Read);

	auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(PackagePath);
	UPackage* NewPackage = CreatePackage(*PackagePath);
	auto NewDataAsset = NewObject<UJsonDataAsset>(NewPackage, AssetClass, *ObjectName, RF_Public | RF_Standalone);

	if (IsValid(NewDataAsset) == false || IsValid(NewPackage) == false)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Error,
			TEXT("Failed to create new object or package for json data asset %s"),
			*PackagePath);
		return false;
	}

	FAssetRegistryModule::AssetCreated(NewDataAsset);

	// Construct a filename from long package name.
	const FString& FileExtension = FPackageName::GetAssetPackageExtension();
	auto PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FileExtension);

	// The file already exists, no need to prompt for save as
	FString BaseFilename, Extension, Directory;
	// Split the path to get the filename without the directory structure
	FPaths::NormalizeFilename(PackageFilename);
	FPaths::Split(PackageFilename, Directory, BaseFilename, Extension);

	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Standalone;
	SaveArgs.Error = GWarn;
	auto SaveResult = UPackage::Save(NewPackage, NewDataAsset, *PackageFilename, SaveArgs);
	if (SaveResult.IsSuccessful() == false)
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to save new package for json data asset %s"), *PackagePath);
		return false;
	}
	if (FPaths::FileExists(FileNameCorrected) == false)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Error,
			TEXT("Failed to export json files for new json data asset %s"),
			*PackagePath);
		return false;
	}
	return true;
}
