// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/ContentBrowserJsonDataSource.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetDependencyGatherer.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetViewUtils.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "ContentBrowserDataSource.h"
#include "ContentBrowserFileDataCore.h"
#include "ContentBrowserFileDataPayload.h"
#include "ContentBrowserModule.h"
#include "Editor.h"
#include "Engine.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "FileHelpers.h"
#include "GameDelegates.h"
#include "HAL/PlatformFile.h"
#include "IContentBrowserDataModule.h"
#include "IContentBrowserSingleton.h"
#include "JsonDataAsset/JsonDataAssetEditor.h"
#include "JsonDataAsset/JsonDataAssetSubsystem.h"
#include "JsonDataAsset/JsonFileSourceControlContextMenu.h"
#include "JsonObjectConverter.h"
#include "Kismet2/SClassPickerDialog.h"
#include "LogOpenUnrealUtilities.h"
#include "Logging/MessageLogMacros.h"
#include "Misc/DataValidation.h"
#include "Misc/FileHelper.h"
#include "Misc/JsonLibrary.h"
#include "Misc/Paths.h"
#include "Modules/ModuleManager.h"
#include "Templates/ScopedAssign.h"
#include "ToolMenus.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/SavePackage.h"

#define JSON_CBROWSER_SOURCE_NAME TEXT("JsonData")

PRAGMA_DISABLE_OPTIMIZATION

class FAssetClassParentFilter : public IClassViewerFilter
{
public:
	/** All children of these classes will be included unless filtered out by another setting. */
	TSet<const UClass*> AllowedChildrenOfClasses;

	/** Disallowed class flags. */
	EClassFlags DisallowedClassFlags;

	virtual bool IsClassAllowed(
		const FClassViewerInitializationOptions& InInitOptions,
		const UClass* InClass,
		TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		return !InClass->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;
	}

	virtual bool IsUnloadedClassAllowed(
		const FClassViewerInitializationOptions& InInitOptions,
		const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
		TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
			&& InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData)
			!= EFilterReturn::Failed;
	}
};

IContentBrowserSingleton& GetContentBrowser()
{
	return FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser").Get();
}

bool UContentBrowserJsonFileDataSource::CanMoveItem(
	const FContentBrowserItemData& InItem,
	const FName InDestPath,
	FText* OutErrorMsg)
{
	if (OutErrorMsg)
	{
		*OutErrorMsg = INVTEXT("Moving source files not implemented. You can move the generated assets.");
	}
	return false;
	/*
	bool bCanMove = true;

	Config.EnumerateFileActions([&](TSharedRef<const ContentBrowserFileData::FFileActions> InFileActions) {
		if (!InFileActions->CanMove.IsBound()
			|| InFileActions->CanMove.Execute(InItem.GetInternalPath(), InItem.GetItemName().ToString(),
	InDestPath.ToString(), OUT OutErrorMsg)
				== false)
		{
			bCanMove = false;
			// break loop
			return false;
		}

		return true;
	});

	return bCanMove;
	*/
}

bool UContentBrowserJsonFileDataSource::MoveItem(const FContentBrowserItemData& InItem, const FName InDestPath)
{
	ensureMsgf(false, TEXT("MoveItem should never be called, because moving is disabled"));
	return Super::MoveItem(InItem, InDestPath);
	// React to items being moved!
}

bool UContentBrowserJsonFileDataSource::BulkMoveItems(
	TArrayView<const FContentBrowserItemData> InItems,
	const FName InDestPath)
{
	ensureMsgf(false, TEXT("BulkMoveItems should never be called, because moving is disabled"));
	return Super::BulkMoveItems(InItems, InDestPath);
	// React to itmes being moved!
}

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
	JsonFileActions.TypeDisplayName = INVTEXT("Json Source File");
	JsonFileActions.TypeShortDescription = INVTEXT("Json file");
	JsonFileActions.TypeFullDescription = INVTEXT("A json data file");
	JsonFileActions.DefaultNewFileName = TEXT("new_json_asset");
	JsonFileActions.TypeColor = FColor(255, 156, 0);
	JsonFileActions.PassesFilter.BindLambda(ItemPassesFilter, true);
	JsonFileActions.GetAttribute.BindLambda(GetItemAttribute);

	// What is this?
	// JsonFileActions.Preview.BindLambda(ItemPreview);

	// For now allow creation in all directories (default if not assigned)
	/*
	JsonFileActions.CanCreate.BindLambda(
		[](const FName InDestFolderPath, const FString& InDestFolder, FText* OutErrorMsg) -> bool {
			return true;
		});
	*/
	JsonFileActions.ConfigureCreation.BindLambda(
		[this](FString& OutFileBasename, FStructOnScope& OutCreationConfig) -> bool {
			OutCreationConfig.Initialize(FOUUJsonDataCreateParams::StaticStruct());
			auto& Config = *(FOUUJsonDataCreateParams*)OutCreationConfig.GetStructMemory();

			// Load the classviewer module to display a class picker
			FClassViewerModule& ClassViewerModule =
				FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

			// Fill in options
			FClassViewerInitializationOptions Options;
			Options.Mode = EClassViewerMode::ClassPicker;

			TSharedRef<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
			Options.ClassFilters.Add(Filter);

			Filter->DisallowedClassFlags =
				CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;
			Filter->AllowedChildrenOfClasses.Add(UJsonDataAsset::StaticClass());

			const FText TitleText = INVTEXT("Pick a Json Data Asset Class");
			UClass* NewChosenClass = nullptr;
			if (SClassPickerDialog::PickClass(TitleText, Options, OUT NewChosenClass, UJsonDataAsset::StaticClass()))
			{
				Config.Class = NewChosenClass;
				return true;
			}

			return false;
		});

	JsonFileActions.Create.BindLambda(
		[this](const FName InFilePath, const FString& InFilenameWrong, const FStructOnScope& CreationConfig) -> bool {
			auto& Config = *(FOUUJsonDataCreateParams*)CreationConfig.GetStructMemory();

			if (IsValid(Config.Class.Get()) == false)
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
			auto FileNameCorrected =
				OUU::Runtime::JsonData::PackageToSourceFull(PackagePath, EJsonDataAccessMode::Read);

			auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(PackagePath);
			UPackage* NewPackage = CreatePackage(*PackagePath);
			auto NewDataAsset =
				NewObject<UJsonDataAsset>(NewPackage, Config.Class, *ObjectName, RF_Public | RF_Standalone);

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
				UE_LOG(
					LogOpenUnrealUtilities,
					Error,
					TEXT("Failed to save new package for json data asset %s"),
					*PackagePath);
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
		});

	JsonFileActions.Edit.BindLambda([this](const FName InFilePath, const FString& InFilename) -> bool {
		auto JsonPath = OUU::Editor::JsonData::ConvertMountedSourceFilenameToDataAssetPath(InFilePath.ToString());
		OUU::Editor::JsonData::ContentBrowser_OpenUnrealEditor(JsonPath);
		return true;
	});

	JsonFileActions.CanMove.BindLambda(
		[](const FName InFilePath, const FString& InFilename, const FString& InDestFolder, FText* OutErrorMsg) -> bool {
			UContentBrowserDataSubsystem* ContentBrowserData = IContentBrowserDataModule::Get().GetSubsystem();
			if (ensure(IsValid(ContentBrowserData)))
			{
				// Redirect to asset (e.g. format
				// "/All/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_AllValuesSet.TestAsset_AllValuesSet")
				auto AssetName = InFilePath;
				auto AssetItem =
					ContentBrowserData->GetItemAtPath(AssetName, EContentBrowserItemTypeFilter::IncludeFiles);
				// return AssetItem.CanMove(*InDestFolder, OutErrorMsg);
			}

			if (OutErrorMsg)
			{
				*OutErrorMsg = INVTEXT("Moving json files is only supported via the generated uassets at the moment");
			}
			return false;
		});

	JsonFileActions.CanRename.BindLambda(
		[](const FName InFilePath, const FString& InFilename, const FString* InNewName, FText* OutErrorMsg) -> bool {
			if (FPaths::DirectoryExists(InFilename) == false && FPaths::FileExists(InFilename) == false)
			{
				// Rename is also used during creation when duplicating
				return true;
			}
			if (OutErrorMsg)
			{
				*OutErrorMsg = INVTEXT("Renaming json files is only supported via the generated uassets at the moment");
			}
			return false;
		});

	JsonFileActions.CanDelete.BindLambda(
		[](const FName InFilePath, const FString& InFilename, FText* OutErrorMsg) -> bool {
			if (OutErrorMsg)
			{
				*OutErrorMsg = INVTEXT("Deleting json files is only supported via the generated uassets at the moment");
			}
			return false;
		});

	JsonFileConfig.RegisterFileActions(JsonFileActions);

	JsonFileDataSource.Reset(
		NewObject<UContentBrowserJsonFileDataSource>(GetTransientPackage(), JSON_CBROWSER_SOURCE_NAME));
	JsonFileDataSource->Initialize(JsonFileConfig);

	TArray<FString> RootPaths;
	FPackageName::QueryRootContentPaths(RootPaths);

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

		// Equivalent of "Common" options
		{
			// "Asset Actions"

			// must haves:
			// - Property Matrix
			// - Asset Reference Viewer

			// nice to haves:
			// - Asset Localization
			// - Replace References
		}
		// No Documentation options

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

#undef JSON_CBROWSER_SOURCE_NAME

PRAGMA_ENABLE_OPTIMIZATION
