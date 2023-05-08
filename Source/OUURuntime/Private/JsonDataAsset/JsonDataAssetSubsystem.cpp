// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAssetSubsystem.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "GameDelegates.h"
#include "Interfaces/IPluginManager.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "LogOpenUnrealUtilities.h"
#include "Logging/MessageLogMacros.h"
#include "Templates/ScopedAssign.h"
#include "UObject/SavePackage.h"

namespace OUU::Runtime::JsonData::Private
{
	TAutoConsoleVariable<bool> CVar_ImportAllAssetsOnStartup(
		TEXT("ouu.JsonData.ImportAllAssetsOnStartup"),
		true,
		TEXT("If true, all json files in the Data directory will be loaded on Editor startup."));

	TAutoConsoleVariable<bool> CVar_PurgeAssetCacheOnStartup(
		TEXT("ouu.JsonData.PurgeAssetCacheOnStartup"),
		false,
		TEXT("If true, all generated uobject asset files for the json files will be forcefully deleted at engine "
			 "startup. If enabled, this happens before the import of all assets. This can help with debugging the "
			 "asset loading."));

	TAutoConsoleVariable<bool> CVar_IgnoreLoadErrorsDuringStartupImport(
		TEXT("ouu.JsonData.IgnoreMissingAssetsDuringInitialImport"),
		false,
		TEXT("If true, all load errors relating to generated UAssets will be ignored during initial load. Warning: "
			 "This might hide wrong asset references to json content!"));

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
} // namespace OUU::Runtime::JsonData::Private

//---------------------------------------------------------------------------------------------------------------------

namespace OUU::Runtime::JsonData::Private
{
	TAutoConsoleVariable<bool> CVar_IgnoreInvalidExtensions(
		TEXT("ouu.JsonData.IgnoreInvalidExtensions"),
		false,
		TEXT("If true, files with invalid extensions inside the Data/ folder will be ignored during 'import all "
			 "assets' calls."));

	FString GDataSource_Uncooked = TEXT("JsonDataSrc/");
	FAutoConsoleVariableRef CVar_DataSource_Uncooked(
		TEXT("ouu.JsonData.SourceUncooked"),
		GDataSource_Uncooked,
		TEXT("Root relative path for uncooked json content. Must differ from cooked root and end in a slash!"),
		ECVF_ReadOnly);

	FString GDataSource_Cooked = TEXT("CookedJsonData/");
	FAutoConsoleVariableRef CVar_DataSource_Cooked(
		TEXT("ouu.JsonData.SourceCooked"),
		GDataSource_Cooked,
		TEXT("Root relative path for cooked json content. Must differ from uncooked root and end in a slash!"),
		ECVF_ReadOnly);

} // namespace OUU::Runtime::JsonData::Private

//---------------------------------------------------------------------------------------------------------------------

namespace OUU::Runtime::JsonData
{
	const FName GameRootName = TEXT("Game");

	FString GetSourceRoot_ProjectRelative(
		const FName& RootName,
		EJsonDataAccessMode AccessMode,
		bool bOverrideUseUncooked = false)
	{
		const auto& SourceMappings = bOverrideUseUncooked
			? UJsonDataAssetSubsystem::Get().GetSourceMappings(false) // false = uncooked
			: UJsonDataAssetSubsystem::Get().GetSourceMappings(AccessMode);

		if (auto* SourceFull = SourceMappings.Find(RootName))
		{
			return *SourceFull;
		}
		ensure(false);
		return "";
	}

	FString GetSourceRoot_Full(const FName& RootName, EJsonDataAccessMode AccessMode)
	{
		auto Path = FPaths::ProjectDir() / GetSourceRoot_ProjectRelative(RootName, AccessMode);
		return Path;
	}

#if WITH_EDITOR
	// The content browser should always display the state of the uncooked source files
	FString GetSourceMountPointRoot_Package(const FName& RootName)
	{
		FString VirtualRoot = UJsonDataAssetSubsystem::Get().GetVirtualRoot(RootName);
		auto MountPoint = VirtualRoot.Replace(TEXT("/JsonData/"), *FString(TEXT("/") + Private::GDataSource_Uncooked));
		return MountPoint;
	}
	FString GetSourceMountPointRoot_DiskFull(const FName& RootName)
	{
		auto Path = FPaths::ProjectDir() / GetSourceRoot_ProjectRelative(RootName, EJsonDataAccessMode::Read, true);
		return Path;
	}
#endif

	FString GetCacheMountPointRoot_Package(const FName& RootName)
	{
		return UJsonDataAssetSubsystem::Get().GetVirtualRoot(RootName);
	}

	FString GetCacheMountPointRoot_DiskFull(const FName& RootName)
	{
		// Save into Save dir, so the packages are not versioned and can safely be deleted on engine startup.
		auto Path = FPaths::ProjectSavedDir() / TEXT("JsonDataCache") / RootName.ToString();
		return Path;
	}

	bool PackageIsJsonData(const FString& PackagePath) { return PackagePath.StartsWith(TEXT("/JsonData/")); }

	FString PackageToDataRelative(const FString& PackagePath)
	{
		auto RootName = UJsonDataAssetSubsystem::Get().GetRootNameForPackagePath(PackagePath);
		return PackagePath.Replace(*GetCacheMountPointRoot_Package(RootName), TEXT("")).Append(TEXT(".json"));
	}

	FString PackageToSourceFull(const FString& PackagePath, EJsonDataAccessMode AccessMode)
	{
		auto RootName = UJsonDataAssetSubsystem::Get().GetRootNameForPackagePath(PackagePath);
		auto Path = FPaths::ProjectDir() / GetSourceRoot_ProjectRelative(RootName, AccessMode)
			/ PackageToDataRelative(PackagePath);
		return Path;
	}

	// Take a path that is relative to the project root and convert it into a package path.
	FString SourceFullToPackage(const FString& FullPath, EJsonDataAccessMode AccessMode)
	{
		auto RootName = UJsonDataAssetSubsystem::Get().GetRootNameForSourcePath(FullPath);

		auto RelativeToSource = FullPath;
		ensure(FPaths::MakePathRelativeTo(
			RelativeToSource,
			*FString(GetSourceRoot_Full(RootName, AccessMode) + TEXT("/"))));

		return GetCacheMountPointRoot_Package(RootName) / RelativeToSource.Replace(TEXT(".json"), TEXT(""));
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

		if (FString::Printf(TEXT("/%s"), *JsonData::Private::GDataSource_Uncooked)
			== JsonData::GetCacheMountPointRoot_Package(JsonData::GameRootName))
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Fatal,
				TEXT("Json Data source directory '%s' must have a different name than asset mount point '%s'"),
				*JsonData::Private::GDataSource_Uncooked,
				*FString(JsonData::GetCacheMountPointRoot_Package(JsonData::GameRootName)));
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

} // namespace OUU::Runtime::JsonData

using namespace OUU::Runtime;

void UJsonDataAssetSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Don't add to plugin root names!!!
	// AllPluginRootNames.Add();
	AllRootNames.Add(OUU::Runtime::JsonData::GameRootName);

	{
		// #TODO-OUU Remove uncooked lookup in cooked build?
		const auto UncookedSourceDir = OUU::Runtime::JsonData::Private::GDataSource_Uncooked;
		AllSourceDirectories_Uncooked.Add(UncookedSourceDir);
		SourceDirectories_Uncooked.Add(OUU::Runtime::JsonData::GameRootName, UncookedSourceDir);
	}
	{
		const auto CookedSourceDir = OUU::Runtime::JsonData::Private::GDataSource_Cooked;
		AllSourceDirectories_Cooked.Add(CookedSourceDir);
		SourceDirectories_Cooked.Add(OUU::Runtime::JsonData::GameRootName, CookedSourceDir);
	}

	JsonData::CheckJsonPaths();

#if WITH_EDITOR
	FEditorDelegates::OnPackageDeleted.AddUObject(this, &UJsonDataAssetSubsystem::HandlePackageDeleted);

	CleanupAssetCache();
#endif

	RegisterMountPoints(OUU::Runtime::JsonData::GameRootName);

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

#if WITH_EDITOR
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	// I know, disabling deprecation warnings is shit, but this is easy to fix when it eventually breaks.
	// And having it delegate based is cleaner (and hopefully possible in 5.2) via ModifyCookDelegate,
	// which seems to be accidentally left private in 5.0 / 5.1
	FGameDelegates::Get().GetCookModificationDelegate().BindUObject(this, &UJsonDataAssetSubsystem::ModifyCook);
	PRAGMA_ENABLE_DEPRECATION_WARNINGS
#endif
}

void UJsonDataAssetSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UnregisterMountPoints(OUU::Runtime::JsonData::GameRootName);

#if WITH_EDITOR
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
	TArray<FString> AllPackageNames;
	bool bIgnoreErrorsDuringImport =
		OUU::Runtime::JsonData::Private::CVar_IgnoreLoadErrorsDuringStartupImport.GetValueOnAnyThread();

	if (bIgnoreErrorsDuringImport)
	{
		IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		for (auto& RootName : AllRootNames)
		{
			auto SourceRoot = JsonData::GetSourceRoot_Full(RootName, EJsonDataAccessMode::Read);
			PlatformFile.IterateDirectoryRecursively(
				*SourceRoot,
				[&AllPackageNames](const TCHAR* FilePath, bool bIsDirectory) -> bool {
					if (bIsDirectory == false)
					{
						auto PackagePath = JsonData::SourceFullToPackage(FilePath, EJsonDataAccessMode::Read);
						AllPackageNames.Add(PackagePath);
					}
					return true;
				});
		}

		// We have to ignore references to generated json packages while doing the initial import.
		// Json assets might reference each other.
		for (auto& PackageName : AllPackageNames)
		{
			FLinkerLoad::AddKnownMissingPackage(*PackageName);
		}
	}

	// Perform the actual import
	for (auto& RootName : AllRootNames)
	{
		ImportAllAssets(RootName, bOnlyMissing);
	}

	if (bIgnoreErrorsDuringImport)
	{
		// Now any further package load errors are valid
		for (auto& PackageName : AllPackageNames)
		{
			FLinkerLoad::RemoveKnownMissingPackage(*PackageName);
		}
	}

	bIsInitialAssetImportCompleted = true;
}

void UJsonDataAssetSubsystem::ImportAllAssets(const FName& RootName, bool bOnlyMissing)
{
	const FString JsonDir = JsonData::GetSourceRoot_Full(RootName, EJsonDataAccessMode::Read);
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
			if (!JsonData::ShouldIgnoreInvalidExtensions())
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
			if (!JsonData::ShouldIgnoreInvalidExtensions())
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

		auto* NewDataAsset = FJsonDataAssetPath::FromPackagePath(PackagePath).ForceReload();
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

void UJsonDataAssetSubsystem::AddPluginDataRoot(const FName& PluginName)
{
	if (AllPluginRootNames.Contains(PluginName))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("Json Data root already registered for plugin %s"),
			*PluginName.ToString());
		return;
	}

	auto& PluginManager = IPluginManager::Get();
	if (auto Plugin = PluginManager.FindPlugin(*PluginName.ToString()))
	{
		const auto PluginBaseDir = Plugin->GetBaseDir();

		AllPluginRootNames.Add(PluginName);
		AllRootNames.Add(PluginName);
		{
			// Uncooked data files are split per plugin
			auto UncookedSourceDir = PluginBaseDir / OUU::Runtime::JsonData::Private::GDataSource_Uncooked;
			FPaths::MakePathRelativeTo(UncookedSourceDir, *FPaths::ProjectDir());
			AllSourceDirectories_Uncooked.Add(UncookedSourceDir);
			SourceDirectories_Uncooked.Add(PluginName, UncookedSourceDir);
		}
		{
			// Cooked data files should go into a subfolder of the regular cooked data folder for easier packaging /
			// redistribution
			const auto CookedSourceDir =
				OUU::Runtime::JsonData::Private::GDataSource_Cooked / TEXT("Plugins") / PluginName.ToString();
			AllSourceDirectories_Cooked.Add(CookedSourceDir);
			SourceDirectories_Cooked.Add(PluginName, CookedSourceDir);
		}

		RegisterMountPoints(PluginName);

		OnNewPluginRootAdded.Broadcast(PluginName);
	}
}

const TMap<FName, FString>& UJsonDataAssetSubsystem::GetSourceMappings(EJsonDataAccessMode AccessMode) const
{
	switch (AccessMode)
	{
	case EJsonDataAccessMode::Read: return GetSourceMappings(OUU::Runtime::JsonData::ShouldReadFromCookedContent());
	case EJsonDataAccessMode::Write: return GetSourceMappings(OUU::Runtime::JsonData::ShouldWriteToCookedContent());
	default: checkf(false, TEXT("Invalid access mode")); return SourceDirectories_Uncooked;
	}
}

const TMap<FName, FString>& UJsonDataAssetSubsystem::GetSourceMappings(bool bUseCookedContent) const
{
	return bUseCookedContent ? SourceDirectories_Cooked : SourceDirectories_Uncooked;
}

const TArray<FString>& UJsonDataAssetSubsystem::GetAllSourceDirectories(EJsonDataAccessMode AccessMode) const
{
	switch (AccessMode)
	{
	case EJsonDataAccessMode::Read:
		return OUU::Runtime::JsonData::ShouldReadFromCookedContent() ? AllSourceDirectories_Cooked
																	 : AllSourceDirectories_Uncooked;
	case EJsonDataAccessMode::Write:
		return OUU::Runtime::JsonData::ShouldWriteToCookedContent() ? AllSourceDirectories_Cooked
																	: AllSourceDirectories_Uncooked;
	default: checkf(false, TEXT("Invalid access mode")); return AllSourceDirectories_Uncooked;
	}
}

FString UJsonDataAssetSubsystem::GetVirtualRoot(const FName& RootName) const
{
	return (RootName == OUU::Runtime::JsonData::GameRootName)
		? TEXT("/JsonData/")
		: FString::Printf(TEXT("/JsonData/Plugins/%s/"), *RootName.ToString());
}

FName UJsonDataAssetSubsystem::GetRootNameForPackagePath(const FString& PackagePath) const
{
	if (PackagePath.StartsWith(TEXT("/JsonData/Plugins/")) == false)
	{
		return OUU::Runtime::JsonData::GameRootName;
	}

	for (auto& RootName : AllPluginRootNames)
	{
		if (PackagePath.StartsWith(GetVirtualRoot(RootName)))
		{
			return RootName;
		}
	}

	return NAME_None;
}

FName UJsonDataAssetSubsystem::GetRootNameForSourcePath(const FString& SourcePath) const
{
	// Assume Game is most common, so test it first
	if (IsPathInSourceDirectoryOfNamedRoot(SourcePath, OUU::Runtime::JsonData::GameRootName))
		return OUU::Runtime::JsonData::GameRootName;

	// Then go through all plugins
	for (auto& PluginName : AllPluginRootNames)
	{
		if (IsPathInSourceDirectoryOfNamedRoot(SourcePath, PluginName))
			return PluginName;
	}

	return NAME_None;
}

bool UJsonDataAssetSubsystem::IsPathInSourceDirectoryOfNamedRoot(const FString& SourcePath, const FName& RootName) const
{
	// Assume cooked is more common (in game runtime)
	if (auto* CookedDir = SourceDirectories_Cooked.Find(RootName))
	{
		if (FPaths::IsUnderDirectory(SourcePath, *(FPaths::ProjectDir() / *CookedDir)))
			return true;
	}

	if (auto* UncookedDir = SourceDirectories_Uncooked.Find(RootName))
	{
		if (FPaths::IsUnderDirectory(SourcePath, *(FPaths::ProjectDir() / *UncookedDir)))
			return true;
	}

	return false;
}

void UJsonDataAssetSubsystem::RegisterMountPoints(const FName& RootName)
{
#if WITH_EDITOR
	FPackageName::RegisterMountPoint(
		JsonData::GetSourceMountPointRoot_Package(RootName),
		JsonData::GetSourceMountPointRoot_DiskFull(RootName));
#endif

	FPackageName::RegisterMountPoint(
		JsonData::GetCacheMountPointRoot_Package(RootName),
		JsonData::GetCacheMountPointRoot_DiskFull(RootName));
}

void UJsonDataAssetSubsystem::UnregisterMountPoints(const FName& RootName)
{
	FPackageName::UnRegisterMountPoint(
		JsonData::GetCacheMountPointRoot_Package(RootName),
		JsonData::GetCacheMountPointRoot_DiskFull(RootName));

#if WITH_EDITOR
	FPackageName::UnRegisterMountPoint(
		JsonData::GetSourceMountPointRoot_Package(RootName),
		JsonData::GetSourceMountPointRoot_DiskFull(RootName));
#endif
}

#if WITH_EDITOR
void UJsonDataAssetSubsystem::CleanupAssetCache()
{
	for (auto& RootName : AllRootNames)
	{
		CleanupAssetCache(RootName);
	}
}

void UJsonDataAssetSubsystem::CleanupAssetCache(const FName& RootName)
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString MountDiskPath = JsonData::GetCacheMountPointRoot_DiskFull(RootName);
	if (PlatformFile.DirectoryExists(*MountDiskPath) == false)
	{
		// No existing cache
		return;
	}

	if (JsonData::Private::CVar_PurgeAssetCacheOnStartup.GetValueOnGameThread())
	{
		// Delete the directory on-disk before mounting the directory to purge all generated uasset files.
		PlatformFile.DeleteDirectoryRecursively(*MountDiskPath);
		return;
	}

	// If clearing the whole cache is disabled, at least stale assets that do not have a corresponding source file must
	// be removed
	auto VisitorLambda = [&PlatformFile, &MountDiskPath, &RootName](const TCHAR* FilePath, bool bIsDirectory) -> bool {
		if (bIsDirectory)
		{
			return true;
		}

		auto FilePathStr = FStringView(FilePath);
		auto AssetExtension = FPackageName::GetAssetPackageExtension();
		if (FilePathStr.EndsWith(AssetExtension) == false)
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Warning,
				TEXT("%s is in json data cache directory, but has unexpected extension. Expected only .uasset "
					 "files."),
				FilePath);
			return true;
		}

		FString RelativePath = FilePath;
		bool bIsRelative = FPaths::MakePathRelativeTo(IN OUT RelativePath, *FString(MountDiskPath + TEXT("/")));
		ensureMsgf(bIsRelative, TEXT("File path %s must be in subdirectory of %s"), *FilePath, *MountDiskPath);
		ensureMsgf(
			RelativePath.StartsWith(TEXT("./")) == false,
			TEXT("%s is expecteed to be a relative path but not with a './' prefix"),
			*RelativePath);

		RelativePath = RelativePath.Mid(0, RelativePath.Len() - AssetExtension.Len());

		auto PackagePath = JsonData::GetCacheMountPointRoot_Package(RootName).Append(RelativePath);

		auto SourcePath = JsonData::PackageToSourceFull(PackagePath, EJsonDataAccessMode::Read);
		if (PlatformFile.FileExists(*SourcePath) == false)
		{
			PlatformFile.DeleteFile(FilePath);
			UE_LOG(
				LogOpenUnrealUtilities,
				Log,
				TEXT("Deleted stale uasset (json source is missing) from json data cache: %s"),
				FilePath);
		}
		else
		{
			auto AssetModTime = PlatformFile.GetTimeStamp(FilePath);
			auto SourceModTime = PlatformFile.GetTimeStamp(*SourcePath);

			if (AssetModTime < SourceModTime)
			{
				PlatformFile.DeleteFile(FilePath);
				UE_LOG(
					LogOpenUnrealUtilities,
					Log,
					TEXT("Deleted outdated uasset (json source is newer) from json data cache: %s"),
					FilePath);
			}
		}

		return true;
	};

	PlatformFile.IterateDirectoryRecursively(*MountDiskPath, VisitorLambda);
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
	IAssetRegistry& AssetRegistry = *IAssetRegistry::Get();
	AssetRegistry.WaitForCompletion();

	ensure(bIsInitialAssetImportCompleted);

	TSet<FName> DependencyPackages;
	for (auto& RootName : AllRootNames)
	{
		ModifyCook(RootName, OUT DependencyPackages);
	}

	for (FName& PackageName : DependencyPackages)
	{
		OutExtraPackagesToCook.Add(PackageName.ToString());
	}

	UE_LOG(
		LogOpenUnrealUtilities,
		Display,
		TEXT("UJsonDataAssetLibrary::ModifyCook - Added %i dependency assets for json assets to cook"),
		DependencyPackages.Num());
}

void UJsonDataAssetSubsystem::ModifyCook(const FName& RootName, TSet<FName>& OutDependencyPackages)
{
	const FString JsonDir_READ = JsonData::GetSourceRoot_Full(RootName, EJsonDataAccessMode::Read);
	if (!FPaths::DirectoryExists(JsonDir_READ))
	{
		return;
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	IAssetRegistry& AssetRegistry = *IAssetRegistry::Get();

	int32 NumJsonDataAssetsAdded = 0;

	auto VisitorLambda = [&PlatformFile,
						  &AssetRegistry,
						  &NumJsonDataAssetsAdded,
						  &OutDependencyPackages](const TCHAR* FilePath, bool bIsDirectory) -> bool {
		if (bIsDirectory)
			return true;

		if (FPaths::GetExtension(FilePath) != TEXT("json"))
			return true;

		if (OUU::Runtime::JsonData::ShouldIgnoreInvalidExtensions() && FPaths::GetBaseFilename(FilePath).Contains("."))
			return true;

		auto PackagePath = JsonData::SourceFullToPackage(FilePath, EJsonDataAccessMode::Read);
		auto Path = FJsonDataAssetPath::FromPackagePath(PackagePath);

		auto* LoadedJsonDataAsset = Path.LoadSynchronous();
		if (!ensure(LoadedJsonDataAsset))
			return true;

		LoadedJsonDataAsset->ExportJsonFile();

		auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(PackagePath);
		FAssetIdentifier AssetIdentifier(*PackagePath, *ObjectName);

		TArray<FAssetIdentifier> Dependencies;
		IAssetRegistry::Get()->GetDependencies(AssetIdentifier, OUT Dependencies);

		for (auto& Dependency : Dependencies)
		{
			if (Dependency.IsPackage())
			{
				auto PackageName = Dependency.PackageName;
				if (JsonData::PackageIsJsonData(PackageName.ToString()) == false)
				{
					// We don't want to add json data assets directly to this list.
					// As they are on the do not cook list, they will throw errors.
					OutDependencyPackages.Add(PackageName);
				}
			}
		}
		NumJsonDataAssetsAdded += 1;

		return true;
	};

	IPlatformFile::FDirectoryVisitorFunc VisitorFunc = VisitorLambda;
	PlatformFile.IterateDirectoryRecursively(*JsonDir_READ, VisitorFunc);

	UE_LOG(
		LogOpenUnrealUtilities,
		Log,
		TEXT("Added %i json data assets for json data root %s"),
		NumJsonDataAssetsAdded,
		*RootName.ToString());
}
#endif
