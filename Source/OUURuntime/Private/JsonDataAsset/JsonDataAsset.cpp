// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAsset.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine.h"
#include "GameDelegates.h"
#include "HAL/PlatformFile.h"
#include "JsonObjectConverter.h"
#include "LogOpenUnrealUtilities.h"
#include "Logging/MessageLogMacros.h"
#include "Misc/FileHelper.h"
#include "Misc/JsonLibrary.h"
#include "Misc/Paths.h"
#include "Templates/ScopedAssign.h"
#include "UObject/ObjectSaveContext.h"
#include "UObject/SavePackage.h"

#if WITH_EDITOR
	#include "Editor.h"
	#include "FileHelpers.h"
	#include "SourceControlHelpers.h"
	#include "AssetViewUtils.h"
	#include "AssetToolsModule.h"
	#include "Misc/DataValidation.h"
#endif

#define JSON_MOUNT_POINT TEXT("/JsonData/")

namespace OUU::Runtime::JsonData::Private
{
	TAutoConsoleVariable<bool> CVar_ImportAllAssetsOnStartup(
		TEXT("ouu.JsonData.ImportAllAssetsOnStartup"),
		true,
		TEXT("If true, all json files in the Data directory will be loaded on Editor startup."));

	// #TODO-jreich
	// We can't disable this atm, because resaving assets is only possible for the newly created assets, not with
	// loaded assets. But with this disabled, we'll have a mix of new/loaded assets that we can't tell apart.
	TAutoConsoleVariable<bool> CVar_PurgeAssetCacheOnStartup(
		TEXT("ouu.JsonData.PurgeAssetCacheOnStartup"),
		false,
		TEXT("If true, all generated uobject asset files for the json files will be forcefully deleted at engine "
			 "startup. If enabled, this happens before the import of all assets. This can help with debugging the "
			 "asset loading."));

	TAutoConsoleVariable<bool> CVar_IgnoreInvalidExtensions(
		TEXT("ouu.JsonData.IgnoreInvalidExtensions"),
		false,
		TEXT("If true, files with invalid extensions inside the Data/ folder will be ignored during 'import all "
			 "assets' calls."));

	FAutoConsoleCommand CCommand_ReimportAllAssets(
		TEXT("ouu.JsonData.ReimportAllAssets"),
		TEXT("Load all json data assets and save them as uassets. This performs the same input that occurs during "
			 "editor startup. Optional parameter [bool]: Reimport only missing files? (default: false)"),
		FConsoleCommandWithArgsDelegate::CreateLambda([](TArray<FString> Args) -> void {
			// Reimport everything by default.
			bool bOnlyMissing = false;
			if (Args.Num() > 0)
			{
				LexFromString(bOnlyMissing, *Args[0]);
			}
			GEngine->GetEngineSubsystem<UJsonDataAssetSubsystem>()->ImportAllAssets(bOnlyMissing);
		}));

	FString GDataSource_Uncooked = TEXT("JsonDataSrc/");
	FAutoConsoleVariableRef CVar_DataSource_Uncooked(
		TEXT("ouu.JsonData.SourceUncooked"),
		GDataSource_Uncooked,
		TEXT("Root relative path for uncooked json content. Must differ from cooked root and end in a slash!"),
		ECVF_ReadOnly);

	FString GDataSource_Cooked = TEXT("CookedJsonData/");
	TAutoConsoleVariable<FString> CVar_DataSource_Cooked(
		TEXT("ouu.JsonData.SourceCooked"),
		GDataSource_Cooked,
		TEXT("Root relative path for cooked json content. Must differ from uncooked root and end in a slash!"),
		ECVF_ReadOnly);

	bool ShouldReadFromCookedContent()
	{
#if WITH_EDITOR
		// Are there cases where we want to read from cooked content in editor? E.g. when running "-game" with
		// cooked content?
		return false;
#else
		return true;
#endif
	}

	bool ShouldWriteToCookedContent()
	{
#if WITH_EDITOR
		// Are there other cases? E.g. when running "-game" with cooked content?
		return IsRunningCookCommandlet();
#else
		return true;
#endif
	}

	void CheckJsonPaths()
	{
#if DO_CHECK
		auto CheckJsonPathFromConfig = [](const FString& Path) {
			if (Path.StartsWith(TEXT("/")))
			{
				UE_LOG(LogOpenUnrealUtilities, Fatal, TEXT("Json path '%s' must not begin with a slash"), *Path);
			}
			if (Path.Contains(TEXT("//")))
			{
				UE_LOG(LogOpenUnrealUtilities, Fatal, TEXT("Json path '%s' must not contain double slashes"), *Path);
			}
			if (Path.EndsWith(TEXT("/")) == false)
			{
				UE_LOG(LogOpenUnrealUtilities, Fatal, TEXT("Json path '%s' does not end in a single slash"), *Path);
			}
		};
		CheckJsonPathFromConfig(JsonData::Private::GDataSource_Uncooked);
		CheckJsonPathFromConfig(JsonData::Private::GDataSource_Cooked);

		if (FString::Printf(TEXT("/%s"), *JsonData::Private::GDataSource_Uncooked) == JSON_MOUNT_POINT)
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Fatal,
				TEXT("Json Data source directory '%s' must have a different name than asset mount point '%s'"),
				*JsonData::Private::GDataSource_Uncooked,
				*FString(JSON_MOUNT_POINT));
		}

		if (JsonData::Private::GDataSource_Uncooked == JsonData::Private::GDataSource_Cooked)
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Fatal,
				TEXT("Cooked and uncooked json paths must differ (%s, %s)"),
				*JsonData::Private::GDataSource_Cooked,
				*JsonData::Private::GDataSource_Uncooked)
		}
#endif
	}
} // namespace OUU::Runtime::JsonData::Private

namespace OUU::Runtime::JsonData
{
	FString GetSourceRoot_ProjectRelative(EJsonDataAccessMode AccessMode)
	{
		switch (AccessMode)
		{
		case EJsonDataAccessMode::Read:
			return Private::ShouldReadFromCookedContent() ? Private::GDataSource_Cooked : Private::GDataSource_Uncooked;
		case EJsonDataAccessMode::Write:
			return Private::ShouldWriteToCookedContent() ? Private::GDataSource_Cooked : Private::GDataSource_Uncooked;
		default: checkf(false, TEXT("Invalid access mode")); return "";
		}
	}

	FString GetSourceRoot_Full(EJsonDataAccessMode AccessMode)
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectDir() / GetSourceRoot_ProjectRelative(AccessMode));
	}

#if WITH_EDITOR
	// The content browser should always display the state of the uncooked source files
	FString GetSourceMountPointRoot_Package() { return TEXT("/") + Private::GDataSource_Uncooked; }
	FString GetSourceMountPointRoot_DiskFull() { return FPaths::ProjectDir() / Private::GDataSource_Uncooked; }
#endif

	// Mount point for generated packages.
	// Save into Save dir, so the packages are not versioned and can safely be deleted on engine startup.
	FString GetCacheMountPointRoot_Package() { return JSON_MOUNT_POINT; }
	FString GetCacheMountPointRoot_DiskFull() { return FPaths::ProjectSavedDir() / TEXT("JsonDataCache/"); }

	bool PackageIsJsonData(const FString& PackagePath)
	{
		return PackagePath.StartsWith(GetCacheMountPointRoot_Package());
	}

	FString PackageToDataRelative(const FString& PackagePath)
	{
		return PackagePath.Replace(JSON_MOUNT_POINT, TEXT("")).Append(TEXT(".json"));
	}

	FString PackageToSourceFull(const FString& PackagePath, EJsonDataAccessMode AccessMode)
	{
		return FPaths::ConvertRelativePathToFull(
			FPaths::ProjectDir() / GetSourceRoot_ProjectRelative(AccessMode) / PackageToDataRelative(PackagePath));
	}

	// Take a path that is relative to the project root and convert it into a package path.
	FString SourceFullToPackage(const FString& FullPath, EJsonDataAccessMode AccessMode)
	{
		return GetCacheMountPointRoot_Package()
			/ FullPath.Replace(*GetSourceRoot_Full(AccessMode), TEXT("")).Replace(TEXT(".json"), TEXT(""));
	}

	FString PackageToObjectName(const FString& Package)
	{
		int32 Idx = INDEX_NONE;
		if (!Package.FindLastChar(TCHAR('/'), OUT Idx))
			return "";
		return Package.RightChop(Idx + 1);
	}

	bool ShouldIgnoreInvalidExtensions()
	{
		return JsonData::Private::CVar_IgnoreInvalidExtensions.GetValueOnAnyThread();
	}
} // namespace OUU::Runtime::JsonData

namespace OUU::Runtime::JsonData::Private
{
	void Delete(const FString& PackagePath)
	{
		auto FullPath = OUU::Runtime::JsonData::PackageToSourceFull(PackagePath, EJsonDataAccessMode::Write);
		if (OUU::Runtime::JsonData::Private::ShouldWriteToCookedContent())
		{
			IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			PlatformFile.DeleteFile(*FullPath);
		}
		else
		{
#if WITH_EDITOR
			USourceControlHelpers::MarkFileForDelete(FullPath);
#else
			UE_LOG(
				LogOpenUnrealUtilities,
				Warning,
				TEXT("Can't delete file '%s' from uncooked content in non-editor context."),
				*FullPath);
#endif
		}
	}
} // namespace OUU::Runtime::JsonData::Private

using namespace OUU::Runtime;

UJsonDataAsset* FJsonDataAssetPath::Resolve() const
{
	const auto ResolvedObject = Path.LoadSynchronous();
	return ResolvedObject ? ResolvedObject : Load();
}

UJsonDataAsset* FJsonDataAssetPath::Load() const
{
	return UJsonDataAssetLibrary::LoadJsonDataAsset(*this);
}

bool FJsonDataAssetPath::ImportTextItem(
	const TCHAR*& Buffer,
	int32 PortFlags,
	UObject* Parent,
	FOutputDevice* ErrorText)
{
	Path = FSoftObjectPath(Buffer);
	return true;
}

bool FJsonDataAssetPath::ExportTextItem(
	FString& ValueStr,
	FJsonDataAssetPath const& DefaultValue,
	UObject* Parent,
	int32 PortFlags,
	UObject* ExportRootScope) const
{
	ValueStr = Path.ToString();
	return true;
}

void UJsonDataAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	JsonData::Private::CheckJsonPaths();

#if WITH_EDITOR
	FEditorDelegates::OnPackageDeleted.AddUObject(this, &UJsonDataAssetSubsystem::HandlePackageDeleted);
#endif
	const FString MountDiskPath = JsonData::GetCacheMountPointRoot_DiskFull();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (JsonData::Private::CVar_PurgeAssetCacheOnStartup.GetValueOnGameThread()
		&& PlatformFile.DirectoryExists(*MountDiskPath))
	{
		// Delete the directory on-disk before mounting the directory to purge all generated uasset files.
		PlatformFile.DeleteDirectoryRecursively(*MountDiskPath);
	}

	FPackageName::RegisterMountPoint(JsonData::GetCacheMountPointRoot_Package(), MountDiskPath);
#if WITH_EDITOR
	FPackageName::RegisterMountPoint(
		JsonData::GetSourceMountPointRoot_Package(),
		JsonData::GetSourceMountPointRoot_DiskFull());
#endif

	if (JsonData::Private::CVar_ImportAllAssetsOnStartup.GetValueOnGameThread())
	{
		auto& AssetRegistry = *IAssetRegistry::Get();
		if (AssetRegistry.IsSearchAllAssets() == false || AssetRegistry.IsLoadingAssets())
		{
			AssetRegistry.OnFilesLoaded().AddUObject(this, &UJsonDataAssetSubsystem::ImportAllAssets, true);
		}
		else
		{
			ImportAllAssets(true);
		}
	}

	bAutoExportJson = true;

	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// I know, disabling deprecation warnings is shit, but this is easy to fix when it eventually breaks.
	// And having it delegate based is cleaner (and hopefully possible in 5.2) via ModifyCookDelegate,
	// which seems to be accidentally left private in 5.0 / 5.1
	FGameDelegates::Get().GetCookModificationDelegate().BindUObject(this, &UJsonDataAssetSubsystem::ModifyCook);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
}

void UJsonDataAssetSubsystem::Deinitialize()
{
	Super::Deinitialize();
	FPackageName::UnRegisterMountPoint(
		JsonData::GetCacheMountPointRoot_Package(),
		JsonData::GetCacheMountPointRoot_DiskFull());

#if WITH_EDITOR

	FPackageName::UnRegisterMountPoint(
		JsonData::GetSourceMountPointRoot_Package(),
		JsonData::GetSourceMountPointRoot_DiskFull());

	FEditorDelegates::OnPackageDeleted.RemoveAll(this);
#endif

	bAutoExportJson = false;
}

bool UJsonDataAssetSubsystem::AutoExportJsonEnabled()
{
	if (IsRunningCookCommandlet())
		return false;
	return GEngine->GetEngineSubsystem<UJsonDataAssetSubsystem>()->bAutoExportJson;
}

void UJsonDataAssetSubsystem::ImportAllAssets(bool bOnlyMissing)
{
	const FString JsonDir = JsonData::GetSourceRoot_Full(EJsonDataAccessMode::Read);
	if (!FPaths::DirectoryExists(JsonDir))
	{
		// No need to import anything if there is no json source directory
		return;
	}

	// Ensure that none of the asset saves during this call scope cause json exports.
	TScopedAssign ScopedDisableAutoExport{this->bAutoExportJson, false};

	TArray<UPackage*> AllPackages;
	int32 NumPackagesLoaded = 0;
	int32 NumPackagesFailedToLoad = 0;
	auto VisitorLambda = [&AllPackages,
						  &NumPackagesLoaded,
						  &NumPackagesFailedToLoad,
						  bOnlyMissing](const TCHAR* FilePath, bool bIsDirectory) -> bool {
		if (bIsDirectory)
			return true;

		if (FPaths::GetExtension(FilePath) != TEXT("json"))
		{
			if (!JsonData::Private::CVar_IgnoreInvalidExtensions.GetValueOnAnyThread())
			{
				UE_MESSAGELOG(
					LoadErrors,
					Warning,
					"File",
					FilePath,
					"in Data directory has an unexpected file extension.");
				NumPackagesFailedToLoad++;
			}
			// Continue with other files anyways
			return true;
		}

		if (FPaths::GetBaseFilename(FilePath).Contains("."))
		{
			if (!JsonData::Private::CVar_IgnoreInvalidExtensions.GetValueOnAnyThread())
			{
				UE_MESSAGELOG(
					LoadErrors,
					Warning,
					"File",
					FilePath,
					"in Data directory has two '.' characters in it's filename. Only a simple '.json' extension is "
					"allowed.");
				NumPackagesFailedToLoad++;
			}
			// Continue with other files anyways
			return true;
		}

		auto PackagePath = JsonData::SourceFullToPackage(FilePath, EJsonDataAccessMode::Read);

		FString PackageFilename;
		const bool bPackageAlreadyExists = FPackageName::DoesPackageExist(PackagePath, &PackageFilename);
		if (bPackageAlreadyExists && bOnlyMissing)
		{
			// Existing asset was found. Skip if only importing missing files.
			return true;
		}

		auto* NewDataAsset = FJsonDataAssetPath::FromPackagePath(PackagePath).Load();
		if (!IsValid(NewDataAsset))
		{
			// Error messages in the load function itself should be sufficient. But it's nice to have a summary
			// metric.
			NumPackagesFailedToLoad++;
			// Continue with other files anyways
			return true;
		}

		UPackage* NewPackage = NewDataAsset->GetPackage();

		// The name of the package
		const FString PackageName = NewPackage->GetName();

		// Construct a filename from long package name.
		const FString& FileExtension = FPackageName::GetAssetPackageExtension();
		PackageFilename = FPackageName::LongPackageNameToFilename(PackagePath, FileExtension);

		// The file already exists, no need to prompt for save as
		FString BaseFilename, Extension, Directory;
		// Split the path to get the filename without the directory structure
		FPaths::NormalizeFilename(PackageFilename);
		FPaths::Split(PackageFilename, Directory, BaseFilename, Extension);

		FSavePackageArgs SaveArgs;
		SaveArgs.TopLevelFlags = RF_Standalone;
		SaveArgs.Error = GWarn;
		auto SaveResult = UPackage::Save(NewPackage, NewDataAsset, *PackageFilename, SaveArgs);

		if (SaveResult == ESavePackageResult::Success)
		{
			NumPackagesLoaded++;
		}
		else
		{
			UE_MESSAGELOG(EditorErrors, Error, "Failed to save package", NewPackage, "for json data asset", FilePath);

			NumPackagesFailedToLoad++;
		}
		return true;
	};

	IPlatformFile::FDirectoryVisitorFunc VisitorFunc = VisitorLambda;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*JsonDir, VisitorFunc);

	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Loaded %i json data assets"), NumPackagesLoaded);
	UE_CLOG(
		NumPackagesFailedToLoad > 0,
		LogOpenUnrealUtilities,
		Error,
		TEXT("Failed to load %i json data assets"),
		NumPackagesFailedToLoad);
}

void UJsonDataAssetSubsystem::HandlePackageDeleted(UPackage* Package)
{
	auto PackagePath = Package->GetPathName();

	if (!JsonData::PackageIsJsonData(PackagePath))
		return;

	JsonData::Private::Delete(PackagePath);
}

void UJsonDataAssetSubsystem::ModifyCook(TArray<FString>& OutExtraPackagesToCook)
{
	const FString JsonDir_READ = JsonData::GetSourceRoot_Full(EJsonDataAccessMode::Read);
	if (!FPaths::DirectoryExists(JsonDir_READ))
	{
		UE_LOG(LogOpenUnrealUtilities, Display, TEXT("UJsonDataAssetLibrary::ModifyCook - No additional assets"));
		return;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IAssetRegistry& AssetRegistry = *IAssetRegistry::Get();

	int32 NumJsonDataAssetsAdded = 0;
	auto VisitorLambda = [&PlatformFile,
						  &AssetRegistry,
						  &NumJsonDataAssetsAdded,
						  &OutExtraPackagesToCook](const TCHAR* FilePath, bool bIsDirectory) -> bool {
		if (bIsDirectory)
			return true;

		if (FPaths::GetExtension(FilePath) != TEXT("json"))
			return true;

		if (FPaths::GetBaseFilename(FilePath).Contains("."))
			return true;

		auto PackagePath = JsonData::SourceFullToPackage(FilePath, EJsonDataAccessMode::Read);
		auto Path = FJsonDataAssetPath::FromPackagePath(PackagePath);

		auto* LoadedJsonDataAsset = Path.Load();
		LoadedJsonDataAsset->ExportJsonFile();

		OutExtraPackagesToCook.Add(PackagePath);
		NumJsonDataAssetsAdded += 1;

		return true;
	};

	IPlatformFile::FDirectoryVisitorFunc VisitorFunc = VisitorLambda;
	PlatformFile.IterateDirectoryRecursively(*JsonDir_READ, VisitorFunc);

	UE_LOG(
		LogOpenUnrealUtilities,
		Display,
		TEXT("UJsonDataAssetLibrary::ModifyCook - Added %i json assets and to cook"),
		NumJsonDataAssetsAdded);
}

UJsonDataAsset* UJsonDataAssetLibrary::LoadJsonDataAsset(FJsonDataAssetPath Path)
{
	return LoadJsonDataAsset_Internal(Path, nullptr);
}

bool UJsonDataAssetLibrary::ReloadJsonDataAsset(UJsonDataAsset* DataAsset)
{
	if (!IsValid(DataAsset))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Cannot reload invalid json data asset"));
		return false;
	}
	auto* Result = LoadJsonDataAsset_Internal(FJsonDataAssetPath(DataAsset), DataAsset);
	return IsValid(Result);
}

UJsonDataAsset* UJsonDataAssetLibrary::LoadJsonDataAsset_Internal(
	FJsonDataAssetPath Path,
	UJsonDataAsset* ExistingDataAsset)
{
	if (Path.IsNull())
	{
		return nullptr;
	}

	const FString InPackagePath = Path.GetPackagePath();
	const FString LoadPath = JsonData::PackageToSourceFull(InPackagePath, EJsonDataAccessMode::Read);

	if (!FPaths::FileExists(LoadPath))
	{
		UE_MESSAGELOG(LoadErrors, Warning, "File", LoadPath, "does not exist");
		return nullptr;
	}

	if (!LoadPath.EndsWith(TEXT(".json")))
	{
		UE_MESSAGELOG(LoadErrors, Warning, "Path", LoadPath, "does not end in '.json'");
		return nullptr;
	}

	FString JsonString;
	if (FFileHelper::LoadFileToString(JsonString, *LoadPath))
	{
		UE_LOG(LogOpenUnrealUtilities, Verbose, TEXT("Loaded %s"), *LoadPath);
	}
	else
	{
		UE_MESSAGELOG(LoadErrors, Error, "Failed to load", LoadPath);
		return nullptr;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonString);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("LoadJsonDataAsset - Unable to parse json=[%s]"), *JsonString);
		UE_MESSAGELOG(
			LoadErrors,
			Error,
			"Failed to parse json for",
			LoadPath,
			". See output log above for more information");
		return nullptr;
	}

	const FString ObjectName = JsonData::PackageToObjectName(InPackagePath);
	FString PackageFilename;

	// Even if existing asset was not passed in, it still might be on disk.
	if (IsValid(ExistingDataAsset) == false && FPackageName::DoesPackageExist(InPackagePath, &PackageFilename))
	{
		TSoftObjectPtr<UJsonDataAsset> ExistingAssetPath(InPackagePath + TEXT(".") + ObjectName);
		ExistingDataAsset = ExistingAssetPath.LoadSynchronous();
	}

	UJsonDataAsset* ExistingOrGeneratedAsset = nullptr;
	bool bCheckClassMatches = true;
	if (IsValid(ExistingDataAsset))
	{
		ExistingOrGeneratedAsset = ExistingDataAsset;
		bCheckClassMatches = true;
	}
	else
	{
		FString ClassName = JsonObject->GetStringField(TEXT("Class"));
		auto* pClass = Cast<UClass>(FSoftObjectPath(ClassName).ResolveObject());
		if (!pClass)
		{
			UE_MESSAGELOG(
				LoadErrors,
				Error,
				"Json file",
				LoadPath,
				"does not have a class field or invalid class name (",
				ClassName,
				")");
			return nullptr;
		}

		if (!pClass->IsChildOf<UJsonDataAsset>())
		{
			UE_MESSAGELOG(
				LoadErrors,
				Error,
				"Class",
				pClass,
				"is not a child of",
				UJsonDataAsset::StaticClass(),
				"- encountered while loading",
				LoadPath);
			return nullptr;
		}

		UPackage* GeneratedPackage = CreatePackage(*InPackagePath);
		ExistingOrGeneratedAsset =
			NewObject<UJsonDataAsset>(GeneratedPackage, pClass, *ObjectName, RF_Public | RF_Standalone);
		// No need to check the class. We already did
		bCheckClassMatches = false;
	}

	checkf(IsValid(ExistingOrGeneratedAsset), TEXT("The json asset is expected to be valid at this point"));
	if (!ExistingOrGeneratedAsset->ImportJson(JsonObject, bCheckClassMatches))
	{
		return nullptr;
	}

	// Modify the asset so it gets dirtied for resave.
	// Do not resave in here, so the same function can be used for un-doable editor imports.
	ExistingOrGeneratedAsset->Modify();

	if (!IsValid(ExistingDataAsset))
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(ExistingOrGeneratedAsset);
	}

	return ExistingOrGeneratedAsset;
}

bool UJsonDataAsset::IsInJsonDataContentRoot() const
{
	return JsonData::PackageIsJsonData(GetPackage()->GetPathName());
}

bool UJsonDataAsset::IsFileBasedJsonAsset() const
{
	return IsAsset();
}

bool UJsonDataAsset::ImportJson(TSharedPtr<FJsonObject> JsonObject, bool bCheckClassMatches)
{
	// ---
	// Header information
	// ---
	if (bCheckClassMatches)
	{
		FString ClassName = JsonObject->GetStringField(TEXT("Class"));
		// Better search for the class instead of mandating a perfect string match
		auto* JsonClass = Cast<UClass>(FSoftObjectPath(ClassName).ResolveObject());
		if (GetClass()->IsChildOf(JsonClass) == false)
		{
			UE_MESSAGELOG(
				LoadErrors,
				Error,
				"Class name in json object (",
				ClassName,
				") does not match class of object",
				this,
				GetClass());
			return false;
		}
	}

	if (JsonObject->HasField(TEXT("EngineVersion")))
	{
		const FString JsonVersionString = JsonObject->GetStringField(TEXT("EngineVersion"));
		const bool bIsLicenseeVersion = JsonObject->GetBoolField(TEXT("IsLicenseeVersion"));
		FEngineVersion JsonVersion;
		if (!FEngineVersion::Parse(JsonVersionString, OUT JsonVersion))
		{
			UE_MESSAGELOG(LoadErrors, Error, "Json file for", this, "has an invalid 'EngineVersion' field value");
			return false;
		}

		uint32 Changelist = JsonVersion.GetChangelist();
		JsonVersion.Set(
			JsonVersion.GetMajor(),
			JsonVersion.GetMinor(),
			JsonVersion.GetPatch(),
			Changelist | (bIsLicenseeVersion ? (1U << 31) : 0),
			JsonVersion.GetBranch());

		if (!FEngineVersion::Current().IsCompatibleWith(JsonVersion))
		{
			UE_MESSAGELOG(
				LoadErrors,
				Error,
				"Json file for",
				this,
				"has an invalid engine version:",
				JsonVersionString,
				"is not compatible with",
				FEngineVersion::Current().ToString(),
				". Last compatible version:",
				FEngineVersion::CompatibleWith().ToString());
			return false;
		}
	}

	// ---
	// Property data
	// ---

	auto Data = JsonObject->GetObjectField(TEXT("Data"));
	if (!Data.IsValid())
	{
		UE_MESSAGELOG(LoadErrors, Error, "Json file for", this, "does not contain a 'Data' field");
		return false;
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(Data.ToSharedRef(), GetClass(), this, 0, 0))
	{
		UE_MESSAGELOG(LoadErrors, Error, this, "Failed to import json 'Data' field into UObject properties");
		return false;
	}

	return true;
}

TSharedRef<FJsonObject> UJsonDataAsset::ExportJson() const
{
	auto Result = MakeShared<FJsonObject>();

	// Header information
	Result->SetStringField(TEXT("Class"), GetClass()->GetFullName().Replace(TEXT("Class "), TEXT("")));
	Result->SetStringField(TEXT("EngineVersion"), FEngineVersion::Current().ToString());
	Result->SetBoolField(TEXT("IsLicenseeVersion"), FEngineVersion::Current().IsLicenseeVersion());

	// Property data
	FOUUJsonLibraryObjectFilter Filter;
	Filter.SubObjectDepthLimit = 0;
	const int64 CheckFlags = EPropertyFlags::CPF_Edit;
	Result->SetObjectField(TEXT("Data"), UOUUJsonLibrary::UObjectToJsonObject(this, Filter, CheckFlags));

	return Result;
}

FString UJsonDataAsset::GetJsonFilePathAbs(EJsonDataAccessMode AccessMode) const
{
	return JsonData::PackageToSourceFull(GetPackage()->GetPathName(), AccessMode);
}

FJsonDataAssetPath UJsonDataAsset::GetPath() const
{
	return FJsonDataAssetPath::FromPackagePath(GetPackage()->GetPathName());
}

bool UJsonDataAsset::ImportJsonFile()
{
	if (IsFileBasedJsonAsset() == false)
	{
		UE_MESSAGELOG(
			AssetTools,
			Error,
			this,
			"does not have an associated json file to import from. Did you try to call ImportJsonFile on a CDO?");
		return false;
	}

	return UJsonDataAssetLibrary::ReloadJsonDataAsset(this);
}

bool UJsonDataAsset::ExportJsonFile() const
{
	if (IsFileBasedJsonAsset() == false)
	{
		UE_MESSAGELOG(
			AssetTools,
			Error,
			this,
			"does not have an associated json file to export to. Did you try to call ExportJsonFile on a CDO?");
		return false;
	}

	if (!IsInJsonDataContentRoot())
	{
		UE_MESSAGELOG(
			AssetTools,
			Error,
			this,
			"is a json data asset, but the generated asset is not located in",
			JsonData::GetCacheMountPointRoot_Package(),
			"content directory. Failed to export json file.");
		return false;
	}

	const FString SavePath = GetJsonFilePathAbs(EJsonDataAccessMode::Write);

	TSharedRef<FJsonObject> JsonObject = ExportJson();
	FString JsonString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(OUT & JsonString);
	if (!FJsonSerializer::Serialize(JsonObject, JsonWriter))
	{
		UE_MESSAGELOG(AssetTools, Error, this, "Failed to serialize json for object properties");
		return false;
	}

#if WITH_EDITOR
	if (JsonData::Private::ShouldWriteToCookedContent() == false)
	{
		USourceControlHelpers::CheckOutFile(SavePath, true);
	}
#endif

	if (!FFileHelper::SaveStringToFile(JsonString, *SavePath))
	{
		UE_MESSAGELOG(AssetTools, Error, this, "Failed to save json string to file", SavePath);
		return false;
	}
	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("ExportJsonFile - Saved %s"), *SavePath);

#if WITH_EDITOR
	if (JsonData::Private::ShouldWriteToCookedContent() == false)
	{
		ensureAlways(USourceControlHelpers::CheckOutOrAddFile(SavePath));
	}
#endif
	return true;
}

bool UJsonDataAsset::Rename(
	const TCHAR* NewName /*= nullptr*/,
	UObject* NewOuter /*= nullptr*/,
	ERenameFlags Flags /*= REN_None*/)
{
	if (IsFileBasedJsonAsset() == false)
	{
		return Super::Rename(NewName, NewOuter, Flags);
	}

#if WITH_EDITOR
	if (!UJsonDataAssetSubsystem::AutoExportJsonEnabled())
	{
		UE_MESSAGELOG(AssetTools, Error, "Can't rename", this, "while auto export to json is disabled.");
		return false;
	}

	return Super::Rename(NewName, NewOuter, Flags);
#else
	// Do not allow renaming outside of the editor
	return false;
#endif
}

void UJsonDataAsset::PostRename(UObject* OldOuter, const FName OldName)
{
#if WITH_EDITOR
	Super::PostRename(OldOuter, OldName);

	if (IsFileBasedJsonAsset() == false)
	{
		return;
	}

	auto OldPackagePathName = OldOuter->GetPathName();
	JsonData::Private::Delete(OldPackagePathName);

	{
		if (UObjectRedirector* Redirector = FindObjectFast<UObjectRedirector>(OldOuter, OldName))
		{
			FScopedSlowTask SlowTask(1, INVTEXT("Fixing up redirectors"));
			SlowTask.MakeDialog();
			SlowTask.EnterProgressFrame(1, INVTEXT("Fixing up referencers..."));
			// Load the asset tools module
			FAssetToolsModule& AssetToolsModule = FAssetToolsModule::GetModule();
			// Do not allow any user choice for this.
			// If we allow a choice it should be before the rename starts in the first place.
			// We can't allow keeping around redirector files.
			bool bCheckoutAndPrompt = false;
			AssetToolsModule.Get()
				.FixupReferencers({Redirector}, bCheckoutAndPrompt, ERedirectFixupMode::DeleteFixedUpRedirectors);
		}
		else
		{
			// not every rename creates redirectors, so this case is ok + expected
		}
	}
#else
	checkf(false, TEXT("Renaming/moving is not allowed outside of the editor, so this should never be called."));
#endif
}

void UJsonDataAsset::PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext)
{
	Super::PostSaveRoot(ObjectSaveContext);
#if WITH_EDITOR
	if (IsFileBasedJsonAsset() && UJsonDataAssetSubsystem::AutoExportJsonEnabled())
	{
		// Only export the json files if the subsystem is fully initialized.
		// Otherwise we resave the newly loaded uassets created from json back to json.
		// Also, during editor startup the source control provider is not fully initialized and we run into other
		// issues.
		ExportJsonFile();
	}
#endif
}

void UJsonDataAsset::PostLoad()
{
	Super::PostLoad();

	// Not called for newly created objects, so we should not have to manually prevent duplicate importing.
	if (bIsInPostLoad == false && IsFileBasedJsonAsset())
	{
		bIsInPostLoad = true;
		ImportJsonFile();
		bIsInPostLoad = false;
	}
}

void UJsonDataAsset::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);

	if (IsFileBasedJsonAsset())
	{
		ExportJsonFile();
	}
}

#if WITH_EDITOR
EDataValidationResult UJsonDataAsset::IsDataValid(class FDataValidationContext& Context)
{
	auto Result = Super::IsDataValid(Context);
	if (!IsInJsonDataContentRoot())
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s is a json data asset, but not located in %s content directory. This will prevent correct json "
				 "data loading!"),
			*GetNameSafe(this),
			*JsonData::GetCacheMountPointRoot_Package())));
		return EDataValidationResult::Invalid;
	}
	return Result;
}
#endif
