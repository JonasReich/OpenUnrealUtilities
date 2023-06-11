// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataAsset.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine.h"
#include "HAL/PlatformFile.h"
#include "JsonDataAsset/JsonDataAssetSubsystem.h"
#include "JsonObjectConverter.h"
#include "LogOpenUnrealUtilities.h"
#include "Logging/MessageLogMacros.h"
#include "Misc/FileHelper.h"
#include "Misc/JsonLibrary.h"
#include "Misc/Paths.h"
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

namespace OUU::Runtime::JsonData::Private
{
	void Delete(const FString& PackagePath)
	{
		auto FullPath = OUU::Runtime::JsonData::PackageToSourceFull(PackagePath, EJsonDataAccessMode::Write);
		if (OUU::Runtime::JsonData::ShouldWriteToCookedContent())
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

FJsonDataCustomVersions::FJsonDataCustomVersions(const TSet<FGuid>& CustomVersionGuids)
{
	for (const auto& Guid : CustomVersionGuids)
	{
		auto OptVersion = FCurrentCustomVersions::Get(Guid);
		if (ensureMsgf(
				OptVersion.IsSet(),
				TEXT("Version GUID '%s' provided for json data asset is not registered as a custom version."),
				*Guid.ToString()))
		{
			VersionsByGuid.Add(Guid, OptVersion.GetValue().Version);
		}
	}
}

int32 FJsonDataCustomVersions::GetCustomVersion(const FGuid& CustomVersionGuid) const
{
	const auto Version = VersionsByGuid.Find(CustomVersionGuid);
	if (ensureMsgf(
			Version,
			TEXT("Tried to access custom version '%s' from json data which was not registered via GetCustomVersions."),
			*CustomVersionGuid.ToString()))
	{
		return *Version;
	}

	return -1;
}

void FJsonDataCustomVersions::EnsureExpectedVersions(const TSet<FGuid>& CustomVersionGuids)
{
	for (const auto& Guid : CustomVersionGuids)
	{
		VersionsByGuid.FindOrAdd(Guid, -1);
	}
}

TSharedPtr<FJsonObject> FJsonDataCustomVersions::ToJsonObject() const
{
	auto JsonObject = MakeShared<FJsonObject>();

	for (const auto& Entry : VersionsByGuid)
	{
		JsonObject->SetNumberField(Entry.Key.ToString(), Entry.Value);
	}

	return JsonObject;
}

void FJsonDataCustomVersions::ReadFromJsonObject(const TSharedPtr<FJsonObject>& JsonObject)
{
	VersionsByGuid.Reset();

	if (JsonObject)
	{
		for (const auto& Entry : JsonObject->Values)
		{
			VersionsByGuid.Add(FGuid(Entry.Key), JsonObject->GetIntegerField(Entry.Key));
		}
	}
}

//---------------------------------------------------------------------------------------------------------------------

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

		// There is a chance in the editor that a blueprint class may get recompiled while a json object is being
		// loaded. In that case	the IsChildOf check will fail and we need to manually check if our current class is the
		// old version of the correct class.
		const bool bIsReinstantiatedBlueprint = GetClass()->HasAnyClassFlags(CLASS_NewerVersionExists) && JsonClass
			&& GetClass()->GetName().Contains(JsonClass->GetName());

		if (GetClass()->IsChildOf(JsonClass) == false && bIsReinstantiatedBlueprint == false)
		{
			UE_MESSAGELOG(
				LoadErrors,
				Error,
				"Class name in json object (",
				ClassName,
				") does not match class of object",
				this,
				" (",
				GetClass(),
				")");
			return false;
		}
	}

	FEngineVersion EngineVersion;
	if (JsonObject->HasField(TEXT("EngineVersion")))
	{
		const FString JsonVersionString = JsonObject->GetStringField(TEXT("EngineVersion"));
		const bool bIsLicenseeVersion = JsonObject->GetBoolField(TEXT("IsLicenseeVersion"));
		if (!FEngineVersion::Parse(JsonVersionString, OUT EngineVersion))
		{
			UE_MESSAGELOG(LoadErrors, Error, "Json file for", this, "has an invalid 'EngineVersion' field value");
			return false;
		}

		uint32 Changelist = EngineVersion.GetChangelist();
		EngineVersion.Set(
			EngineVersion.GetMajor(),
			EngineVersion.GetMinor(),
			EngineVersion.GetPatch(),
			Changelist | (bIsLicenseeVersion ? (1U << 31) : 0),
			EngineVersion.GetBranch());

		if (!FEngineVersion::Current().IsCompatibleWith(EngineVersion))
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

	FJsonDataCustomVersions CustomVersions;
	const TSharedPtr<FJsonObject>* ppCustomVersionsObject = nullptr;
	if (JsonObject->TryGetObjectField(TEXT("CustomVersions"), ppCustomVersionsObject))
	{
		CustomVersions.ReadFromJsonObject(*ppCustomVersionsObject);
	}

	CustomVersions.EnsureExpectedVersions(GetRelevantCustomVersions());

	// ---
	// Property data
	// ---

	auto Data = JsonObject->GetObjectField(TEXT("Data"));
	if (!Data.IsValid())
	{
		UE_MESSAGELOG(LoadErrors, Error, "Json file for", this, "does not contain a 'Data' field");
		return false;
	}

	// Reset object properties to class defaults
	{
		auto* CDO = GetClass()->GetDefaultObject();

		for (auto Property : TFieldRange<FProperty>(GetClass(), EFieldIterationFlags::IncludeAll))
		{
			void* SelfPropertyPtr = Property->ContainerPtrToValuePtr<void>(this);
			const void* CDOPropertyPtr = Property->ContainerPtrToValuePtr<void>(CDO);

			if (SelfPropertyPtr && CDOPropertyPtr)
			{
				Property->CopyCompleteValue(SelfPropertyPtr, CDOPropertyPtr);
			}
			else
			{
				UE_LOG(
					LogOpenUnrealUtilities,
					Error,
					TEXT("Property %s in class %s could not be resolved to value pointers on object %s or CDO %s"),
					*Property->GetName(),
					*GetClass()->GetName(),
					*GetName(),
					*CDO->GetName());
			}
		}
	}

	if (!FJsonObjectConverter::JsonObjectToUStruct(Data.ToSharedRef(), GetClass(), this, 0, 0))
	{
		UE_MESSAGELOG(LoadErrors, Error, this, "Failed to import json 'Data' field into UObject properties");
		return false;
	}

	return PostLoadJsonData(EngineVersion, CustomVersions, Data.ToSharedRef());
}

TSharedRef<FJsonObject> UJsonDataAsset::ExportJson() const
{
	auto Result = MakeShared<FJsonObject>();

	// Header information
	{
		Result->SetStringField(TEXT("Class"), GetClass()->GetPathName());
		Result->SetStringField(TEXT("EngineVersion"), FEngineVersion::Current().ToString());
		Result->SetBoolField(TEXT("IsLicenseeVersion"), FEngineVersion::Current().IsLicenseeVersion());

		const FJsonDataCustomVersions CustomVersions(GetRelevantCustomVersions());
		Result->SetObjectField(TEXT("CustomVersions"), CustomVersions.ToJsonObject());
	}

	// Property data
	{
		FOUUJsonLibraryObjectFilter Filter;
		Filter.SubObjectDepthLimit = 0;
		const int64 CheckFlags = EPropertyFlags::CPF_Edit;

		// Data going into the cooked content directory should write all properties into the files to have a baseline
		// for modders. Data going into the regular editor saves should perform delta serialization to support
		// propagation of values from base class defaults.
		bool bOnlyModifiedProperties = OUU::Runtime::JsonData::ShouldWriteToCookedContent() == false;

		Result->SetObjectField(
			TEXT("Data"),
			UOUUJsonLibrary::UObjectToJsonObject(this, Filter, CheckFlags, 0, bOnlyModifiedProperties));
	}

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

	auto ThisIfSuccess = LoadJsonDataAsset_Internal(GetPath(), this);
	if (IsValid(ThisIfSuccess))
	{
		ensureMsgf(
			this == ThisIfSuccess,
			TEXT("Importing json file was successful, but returned a different object. Should always return this or "
				 "nullptr."));
		return true;
	}
	return false;
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
			"is a json data asset, but the generated asset is not located in /JsonData/ content directory. Failed to "
			"export json file.");
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
	if (JsonData::ShouldWriteToCookedContent() == false)
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
	if (JsonData::ShouldWriteToCookedContent() == false)
	{
		ensureAlways(USourceControlHelpers::CheckOutOrAddFile(SavePath));
	}
#endif
	return true;
}

bool UJsonDataAsset::PostLoadJsonData(
	const FEngineVersion& EngineVersion,
	const FJsonDataCustomVersions& CustomVersions,
	TSharedRef<FJsonObject> JsonObject)
{
	return true;
}

bool UJsonDataAsset::MustHandleRename(UObject* OldOuter, const FName OldName) const
{
	if (IsFileBasedJsonAsset() == false)
	{
		// Never need to handle renames of non-file json assets
		return false;
	}
	const auto NewOuter = GetOuter();
	if (NewOuter == OldOuter)
	{
		// From our observation, every "real rename" is accompanied by a change in outer
		return false;
	}

	return (OldOuter == nullptr || NewOuter == nullptr || OldOuter->GetPathName() != NewOuter->GetPathName());
}

TSet<FGuid> UJsonDataAsset::GetRelevantCustomVersions() const
{
	return {};
}

UJsonDataAsset* UJsonDataAsset::LoadJsonDataAsset_Internal(FJsonDataAssetPath Path, UJsonDataAsset* ExistingDataAsset)
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

	if (!IsValid(ExistingDataAsset))
	{
		// Notify the asset registry
		FAssetRegistryModule::AssetCreated(ExistingOrGeneratedAsset);
	}

	return ExistingOrGeneratedAsset;
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

	// We only need to remove the old json file if our outer (the package) or its path has changed. Otherwise the file
	// can stay where it is. When the package is renamed in the editor, we are (at least from my testing) always
	// assigned a new outer.
	if (MustHandleRename(OldOuter, OldName) == false)
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

bool UJsonDataAsset::IsFullNameStableForNetworking() const
{
	return false;
}

bool UJsonDataAsset::IsSupportedForNetworking() const
{
	return false;
}

#if WITH_EDITOR
EDataValidationResult UJsonDataAsset::IsDataValid(class FDataValidationContext& Context)
{
	auto Result = Super::IsDataValid(Context);
	if (!IsInJsonDataContentRoot())
	{
		Context.AddError(FText::FromString(FString::Printf(
			TEXT("%s is a json data asset, but not located in /JsonData/ content directory. This will prevent correct "
				 "json data loading!"),
			*GetNameSafe(this))));
		return EDataValidationResult::Invalid;
	}

	// Check if there are any hard package refs to either the package or object.
	// Both are NEVER permitted, as we only allow referencing via json data asset path, which produces a soft object
	// reference.
	{
		TArray<FAssetIdentifier> PackageReferencers;
		IAssetRegistry::Get()->GetReferencers(
			FAssetIdentifier(GetOutermost()->GetFName()),
			OUT PackageReferencers,
			UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::EDependencyQuery::Hard);
		for (auto& Referencer : PackageReferencers)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s has hard reference to PACKAGE %s"),
				*Referencer.ToString(),
				*GetOutermost()->GetName())));
		}

		TArray<FAssetIdentifier> ObjectReferencers;
		IAssetRegistry::Get()->GetReferencers(
			FAssetIdentifier(GetFName()),
			OUT ObjectReferencers,
			UE::AssetRegistry::EDependencyCategory::Package,
			UE::AssetRegistry::EDependencyQuery::Hard);
		for (auto& Referencer : ObjectReferencers)
		{
			Context.AddError(FText::FromString(FString::Printf(
				TEXT("%s has hard reference to OBJECT %s"),
				*Referencer.ToString(),
				*GetOutermost()->GetName())));
		}
	}

	return Result;
}
#endif
