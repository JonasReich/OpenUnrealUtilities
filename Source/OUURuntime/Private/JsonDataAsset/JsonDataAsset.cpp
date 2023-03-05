// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAsset.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine.h"
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
#endif

// #TODO-jreich Add command to force reload all assets from json files after editor startup.

namespace OUU::Runtime::Private
{
	// We can't disable this atm, because resaving assets is only possible for the newly created assets, not with loaded
	// assets. But with this disabled, we'll have a mix of new/loaded assets that we can't tell apart.
	TAutoConsoleVariable<bool> CVar_PurgeJsonDataFilesOnStartup(
		TEXT("ouu.JsonData.CleanOnStartup"),
		true,
		TEXT("If true, all generated uobject asset files for the json files will be forcefully deleted at engine "
			 "startup. This can help with debugging the asset loading."));

	// Something like this might be needed for runtime package creation in non-editor builds
	// where we cannot save packages.
	UPackage* OUU_CreatePackage(FString PackagePath)
	{
		auto* GeneratedPackage = NewObject<UPackage>(nullptr, *PackagePath, RF_Public | RF_Standalone);
		// reference viewer needs info that is generated on save, but this prevents saving
		// Maybe we can somehow generate that data in code and embed it in the package w/e?
		// GeneratedPackage->SetPackageFlags(PKG_CompiledIn);
		GeneratedPackage->AddToRoot();

		return GeneratedPackage;
	}

	// Mount point for generated packages.
	// Save into Save dir, so the packages are not versioned and can safely be deleted on engine startup.
	FString GetJsonDataMountPoint_Root() { return TEXT("/JsonData/"); }
	FString GetJsonDataMountPoint_Disk() { return FPaths::ProjectSavedDir() / TEXT("JsonData/"); }
	FString GetJsonDataPath_Disk()
	{
		return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / TEXT("../Data/"));
	}

	namespace JsonDataPath
	{
		bool PackageIsJsonData(const FString& Package) { return Package.StartsWith(GetJsonDataMountPoint_Root()); }

		FString PackageToDataRelative(const FString& Package)
		{
			return Package.Replace(TEXT("/JsonData/"), TEXT("")).Append(TEXT(".json"));
		}

		FString PackageToContentRelative(const FString& Package)
		{
			return FString(TEXT("../Data")) / PackageToDataRelative(Package);
		}

		FString PackageToFull(const FString& Package)
		{
			return FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir() / PackageToContentRelative(Package));
		}

		FString FullToPackage(const FString& Full)
		{
			return GetJsonDataMountPoint_Root()
				/ Full.Replace(*GetJsonDataPath_Disk(), TEXT("")).Replace(TEXT(".json"), TEXT(""));
		}

		FString PackageToObjectName(const FString& Package)
		{
			int32 Idx = INDEX_NONE;
			if (!Package.FindLastChar(TCHAR('/'), OUT Idx))
				return "";
			return Package.RightChop(Idx + 1);
		}

		FString PackageToFullObjectName(const FString& Package)
		{
			return FString(Package).Append(TEXT(".")).Append(PackageToObjectName(Package));
		}
	} // namespace JsonDataPath

	void DeleteJsonAsset(const FString& PackagePath)
	{
#if WITH_EDITOR
		// SourceControlHelpers::MarkFileForDelete

		auto FullPath = OUU::Runtime::Private::JsonDataPath::PackageToFull(PackagePath);
		USourceControlHelpers::MarkFileForDelete(FullPath);
#endif
	}

} // namespace OUU::Runtime::Private

UJsonDataAsset* FJsonDataAssetPath::Load()
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

#if WITH_EDITOR
	FEditorDelegates::OnPackageDeleted.AddUObject(this, &UJsonDataAssetSubsystem::HandlePackageDeleted);
#endif

	const FString MountDiskPath = OUU::Runtime::Private::GetJsonDataMountPoint_Disk();
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	if (OUU::Runtime::Private::CVar_PurgeJsonDataFilesOnStartup.GetValueOnGameThread()
		&& PlatformFile.DirectoryExists(*MountDiskPath))
	{
		// Delete the directory on-disk before mounting the directory to purge all generated uasset files.
		PlatformFile.DeleteDirectoryRecursively(*MountDiskPath);
	}

	FPackageName::RegisterMountPoint(OUU::Runtime::Private::GetJsonDataMountPoint_Root(), MountDiskPath);

	ImportAllAssetsDuringStartup();

	bAutoExportJson = true;
}

void UJsonDataAssetSubsystem::Deinitialize()
{
	Super::Deinitialize();
	FPackageName::UnRegisterMountPoint(
		OUU::Runtime::Private::GetJsonDataMountPoint_Root(),
		OUU::Runtime::Private::GetJsonDataMountPoint_Disk());

#if WITH_EDITOR
	FEditorDelegates::OnPackageDeleted.RemoveAll(this);
#endif

	bAutoExportJson = false;
}

bool UJsonDataAssetSubsystem::AutoExportJsonEnabled()
{
	return GEngine->GetEngineSubsystem<UJsonDataAssetSubsystem>()->bAutoExportJson;
}

void UJsonDataAssetSubsystem::ImportAllAssetsDuringStartup()
{
	const FString JsonDir = OUU::Runtime::Private::GetJsonDataPath_Disk();
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
	auto VisitorLambda =
		[&AllPackages, &NumPackagesLoaded, &NumPackagesFailedToLoad](const TCHAR* FilePath, bool bIsDirectory) -> bool {
		if (!bIsDirectory)
		{
			auto Path =
				FJsonDataAssetPath::FromPackagePath(OUU::Runtime::Private::JsonDataPath::FullToPackage(FilePath));
			auto* NewDataAsset = Path.Load();
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

			// Place were we should save the file, including the filename
			FString FinalPackageSavePath;
			// Just the filename
			FString FinalPackageFilename;

			FString PackageFilename;
			const bool bPackageAlreadyExists = FPackageName::DoesPackageExist(PackageName, &PackageFilename);
			if (!bPackageAlreadyExists)
			{
				// Construct a filename from long package name.
				const FString& FileExtension = FPackageName::GetAssetPackageExtension();
				PackageFilename = FPackageName::LongPackageNameToFilename(PackageName, FileExtension);
			}

			// The file already exists, no need to prompt for save as
			FString BaseFilename, Extension, Directory;
			// Split the path to get the filename without the directory structure
			FPaths::NormalizeFilename(PackageFilename);
			FPaths::Split(PackageFilename, Directory, BaseFilename, Extension);
			// The final save path is whatever the existing filename is
			FinalPackageSavePath = PackageFilename;
			// Format the filename we found from splitting the path
			FinalPackageFilename = FString::Printf(TEXT("%s.%s"), *BaseFilename, *Extension);

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
				UE_MESSAGELOG(
					EditorErrors,
					Error,
					"Failed to save package",
					NewPackage,
					"for json data asset",
					FilePath);

				NumPackagesFailedToLoad++;
			}
		}
		return true;
	};

	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Loaed %i json data assets"), NumPackagesLoaded);
	UE_CLOG(
		NumPackagesFailedToLoad > 0,
		LogOpenUnrealUtilities,
		Error,
		TEXT("Failed to load %i json data assets"),
		NumPackagesFailedToLoad);

	IPlatformFile::FDirectoryVisitorFunc VisitorFunc = VisitorLambda;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	PlatformFile.IterateDirectoryRecursively(*JsonDir, VisitorFunc);
}

void UJsonDataAssetSubsystem::HandlePackageDeleted(UPackage* Package)
{
	auto PackagePath = Package->GetPathName();

	if (!OUU::Runtime::Private::JsonDataPath::PackageIsJsonData(PackagePath))
		return;

	OUU::Runtime::Private::DeleteJsonAsset(PackagePath);
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
	UJsonDataAsset* const ExistingDataAsset)
{
	const FString InPackagePath = Path.GetPackagePath();
	const FString LoadPath = OUU::Runtime::Private::JsonDataPath::PackageToFull(InPackagePath);

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

	UJsonDataAsset* GeneratedAsset = nullptr;
	bool bCheckClassMatches = true;
	if (IsValid(ExistingDataAsset))
	{
		GeneratedAsset = ExistingDataAsset;
		bCheckClassMatches = true;
	}
	else
	{
		FString ClassName = JsonObject->GetStringField(TEXT("Class"));
		auto* Class = FindObject<UClass>(nullptr, *ClassName);
		if (!Class)
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

		if (!Class->IsChildOf<UJsonDataAsset>())
		{
			UE_MESSAGELOG(
				LoadErrors,
				Error,
				"Class",
				Class,
				"is not a child of",
				UJsonDataAsset::StaticClass(),
				"- encountered while loading",
				LoadPath);
			return nullptr;
		}

		UPackage* GeneratedPackage = CreatePackage(*InPackagePath);
		const FString ObjectName = OUU::Runtime::Private::JsonDataPath::PackageToObjectName(InPackagePath);
		GeneratedAsset = FindObject<UJsonDataAsset>(GeneratedPackage, *ObjectName);
		if (GeneratedAsset == nullptr)
		{
			GeneratedAsset = NewObject<UJsonDataAsset>(GeneratedPackage, Class, *ObjectName, RF_Public | RF_Standalone);
		}
		// No need to check the class. We already did
		bCheckClassMatches = false;
	}

	checkf(IsValid(GeneratedAsset), TEXT("The json asset is expected to be valid at this point"));
	if (!GeneratedAsset->ImportJson(JsonObject, bCheckClassMatches))
	{
		return nullptr;
	}

	// Modify the asset so it gets dirtied for resave.
	// Do not resave in here, so the same function can be used for un-doable editor imports.
	// #TODO Check if this works as expected
	GeneratedAsset->Modify();

	if (!IsValid(ExistingDataAsset))
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(GeneratedAsset);
	}

	return GeneratedAsset;
}

bool UJsonDataAsset::ImportJson(TSharedPtr<FJsonObject> JsonObject, bool bCheckClassMatches)
{
	if (bCheckClassMatches)
	{
		FString ClassName = JsonObject->GetStringField(TEXT("Class"));
		// Better search for the class instead of mandating a perfect string match
		auto* JsonClass = FindObject<UClass>(nullptr, *ClassName);
		if (JsonClass != GetClass())
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
	// #TODO Can we get the fully qualified class name without the Class prefix differently? Maybe
	// FSoftClassPtr().ToString()?
	Result->SetStringField(TEXT("Class"), GetClass()->GetFullName().Replace(TEXT("Class "), TEXT("")));

	Result->SetStringField(TEXT("EngineVersion"), FEngineVersion::Current().ToString());
	Result->SetBoolField(TEXT("IsLicenseeVersion"), FEngineVersion::Current().IsLicenseeVersion());

	FOUUJsonLibraryObjectFilter Filter;
	Filter.SubObjectDepthLimit = 0;
	const int64 CheckFlags = EPropertyFlags::CPF_Edit;
	Result->SetObjectField(TEXT("Data"), UOUUJsonLibrary::UObjectToJsonObject(this, Filter, CheckFlags));

	return Result;
}

FString UJsonDataAsset::GetJsonFilePathAbs() const
{
	return OUU::Runtime::Private::JsonDataPath::PackageToFull(GetPackage()->GetPathName());
}

bool UJsonDataAsset::ImportJsonFile()
{
	return UJsonDataAssetLibrary::ReloadJsonDataAsset(this);
}

bool UJsonDataAsset::ExportJsonFile() const
{
	// Const cast is permissible here. We just want to reuse the path conversion.
	const FString InPackagePath = FJsonDataAssetPath(const_cast<UJsonDataAsset*>(this)).GetPackagePath();
	const FString SavePath = OUU::Runtime::Private::JsonDataPath::PackageToFull(InPackagePath);

	TSharedRef<FJsonObject> JsonObject = ExportJson();
	FString JsonString;
	TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(OUT & JsonString);
	if (!FJsonSerializer::Serialize(JsonObject, JsonWriter))
	{
		UE_MESSAGELOG(AssetTools, Error, this, "Failed to serialize json for object properties");
		return false;
	}

#if WITH_EDITOR
	USourceControlHelpers::CheckOutFile(SavePath, true);
#endif

	if (!FFileHelper::SaveStringToFile(JsonString, *SavePath))
	{
		UE_MESSAGELOG(AssetTools, Error, this, "Failed to save json string to file", SavePath);
		return false;
	}
	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("ExportJsonFile - Saved %s"), *SavePath);

#if WITH_EDITOR
	ensureAlways(USourceControlHelpers::CheckOutOrAddFile(SavePath));
#endif
	return true;
}

bool UJsonDataAsset::Rename(
	const TCHAR* NewName /*= nullptr*/,
	UObject* NewOuter /*= nullptr*/,
	ERenameFlags Flags /*= REN_None*/)
{
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

	auto OldPackagePathName = OldOuter->GetPathName();
	OUU::Runtime::Private::DeleteJsonAsset(OldPackagePathName);

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
	if (UJsonDataAssetSubsystem::AutoExportJsonEnabled())
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
	ImportJsonFile();
}

void UJsonDataAsset::PostDuplicate(bool bDuplicateForPIE)
{
	Super::PostDuplicate(bDuplicateForPIE);
	ExportJsonFile();
}
