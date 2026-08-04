// Copyright (c) 2023 Jonas Reich & Contributors

#include "Commandlets/OUUValidateAssetListCommandlet.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetValidation/OUUAssetValidationSettings.h"
#include "Dom/JsonObject.h"
#include "Editor.h"
#include "Editor/Transactor.h"
#include "EditorValidatorSubsystem.h"
#include "FileHelpers.h"
#include "IMessageLogListing.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompilerModule.h"
#include "LogOpenUnrealUtilities.h"
#include "MessageLogModule.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"

namespace OUU::Editor::ValidateAssetList::Private
{
	// Releases the currently loaded editor map and forces a garbage collection.
	//
	// Validating a world asset loads a full map and can trigger world partition streaming generation
	// as well as blueprint reinstancing (e.g. when the project-wide WorldSettings blueprint is compiled).
	// Inside a single commandlet process none of that is collected on its own, so leftover duplicated /
	// reinstanced actors keep piling up in memory. In particular the WorldSettings actor can survive as a
	// duplicate and then get misreported as "has same GUID as ..." by a later MAP CHECK - a false positive
	// that never shows in the editor, because there only ever one world is resident and GC runs on map change.
	//
	// Switching to a blank map first makes the world we just validated unreferenced so the GC can actually
	// reclaim it, keeping memory bounded and stopping stale actors from leaking into subsequent map checks.
	void PurgeLoadedMap()
	{
		UEditorLoadingAndSavingUtils::NewBlankMap(/*bSaveExistingMap*/ false);

		// The transaction buffer can keep the trashed world and its actors alive, defeating the GC.
		if (GEditor->Trans != nullptr)
		{
			GEditor->Trans->Reset(NSLOCTEXT("OUUValidateAssetList", "PurgeLoadedMap", "Asset validation"));
		}

		CollectGarbage(RF_NoFlags);
	}
} // namespace OUU::Editor::ValidateAssetList::Private

int32 UOUUValidateAssetListCommandlet::Main(const FString& FullCommandLine)
{
	FString AssetListFilePath;
	FParse::Value(*FullCommandLine, TEXT("AssetList="), AssetListFilePath);
	FString ValidationReportPath;
	FParse::Value(*FullCommandLine, TEXT("ValidationReport="), ValidationReportPath);

	TArray<FAssetData> AssetDataList;

	FString AssetListString;
	if (FFileHelper::LoadFileToString(AssetListString, *AssetListFilePath) == false)
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to read asset list %s"), *AssetListFilePath);
		return -1;
	}

	const UEditorValidatorSubsystem* EditorValidationSubsystem =
		GEditor->GetEditorSubsystem<UEditorValidatorSubsystem>();
	check(EditorValidationSubsystem);

	// Just make sure the kismet compiler module is properly loaded
	FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(TEXT(KISMET_COMPILER_MODULENAME));

	const auto& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Make sure asset registry is fully loaded before validation, some assets may rely on that.
	if (UE::AssetRegistry::ShouldSearchAllAssetsAtStart() == false)
	{
		AssetRegistry.SearchAllAssets(true);
		// Note: OnFilesLoaded will never be broadcast if the asset registry doesn't search all assets right from the
		// start, so we have to trigger that manually. Why?
		AssetRegistry.OnFilesLoaded().Broadcast();
	}
	else
	{
		AssetRegistry.WaitForCompletion();
	}

	const auto ReportObject = MakeShared<FJsonObject>();
	int32 NumInvalidAssets = 0;

	// Paths from file
	{
		const TCHAR* AssetListBuffer = *AssetListString;
		FString AssetListEntry;
		while (FParse::Line(&AssetListBuffer, OUT AssetListEntry))
		{
			ValidateEntry(
				*EditorValidationSubsystem,
				AssetRegistry,
				AssetListEntry,
				OUT ReportObject,
				OUT NumInvalidAssets);
		}
	}

	// Always include dirs
	for (auto& Entry : UOUUAssetValidationSettings::Get().AssetListValidation_AlwaysIncludeDirectories)
	{
		auto Path = Entry.Path;
		if (Path.EndsWith(TEXT("/")) == false)
		{
			Path += TEXT("/");
		}
		ValidateEntry(*EditorValidationSubsystem, AssetRegistry, Path, OUT ReportObject, OUT NumInvalidAssets);
	}

	// Always include assets
	for (auto& Entry : UOUUAssetValidationSettings::Get().AssetListValidation_AlwaysIncludeAssets)
	{
		ValidateEntry(
			*EditorValidationSubsystem,
			AssetRegistry,
			Entry.GetLongPackageName(),
			OUT ReportObject,
			OUT NumInvalidAssets);
	}

	FString JsonString;
	const TSharedRef<TJsonWriter<>> JsonWriter = TJsonWriterFactory<>::Create(OUT & JsonString);
	ensure(FJsonSerializer::Serialize(ReportObject, JsonWriter));
	ensure(FFileHelper::SaveStringToFile(JsonString, *ValidationReportPath));

	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("Wrote validation report to %s"), *ValidationReportPath);
	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("%i invalid assets"), NumInvalidAssets);

	return NumInvalidAssets;
}

void UOUUValidateAssetListCommandlet::ValidateEntry(
	const UEditorValidatorSubsystem& EditorValidationSubsystem,
	const IAssetRegistry& AssetRegistry,
	const FString& AssetListEntry,
	const TSharedRef<FJsonObject>& OutReportObject,
	int32& OutNumInvalidAssets)
{
	TArray<FAssetData> PackageAssetsData;

	// Automatically detect folders by their trailing slash
	if (AssetListEntry.EndsWith(TEXT("/")))
	{
		auto PathWithoutSuffix = AssetListEntry;
		PathWithoutSuffix.RemoveFromEnd(TEXT("/"));
		// Asset Registry needs the path without trailing slash though
		AssetRegistry.GetAssetsByPath(*PathWithoutSuffix, OUT PackageAssetsData, true);
	}
	else
	{
		AssetRegistry.GetAssetsByPackageName(*AssetListEntry, OUT PackageAssetsData);
	}

	FMessageLogModule& MessageLogModule = FModuleManager::LoadModuleChecked<FMessageLogModule>("MessageLog");
	const auto MapCheckListing = MessageLogModule.GetLogListing(TEXT("MapCheck"));

	for (auto& Asset : PackageAssetsData)
	{
		ValidateSingleAsset(EditorValidationSubsystem, MapCheckListing, Asset, OutReportObject, OutNumInvalidAssets);
	}
}

void UOUUValidateAssetListCommandlet::ValidateSingleAsset(
	const UEditorValidatorSubsystem& EditorValidationSubsystem,
	const TSharedRef<IMessageLogListing>& MapCheckListing,
	const FAssetData& Asset,
	const TSharedRef<FJsonObject>& OutReportObject,
	int32& OutNumInvalidAssets)
{
	// Objects may appear via always validate AND recently changed lists, so need to be filtered for duplicates.
	if (OutReportObject->FJsonObject::HasField(Asset.PackageName.ToString()))
	{
		return;
	}

	MapCheckListing->ClearMessages();
	TArray<FString> AssetErrorStrings;

	// Load the asset but make sure we don't accidentally load a whole World partition map with all actors.
	auto* AssetPtr = Asset.GetAsset({ULevel::DontLoadExternalObjectsTag});

	if (const auto* AssetClass = Asset.GetClass())
	{
		if (AssetClass->IsChildOf<UWorld>())
		{
			if (auto* LoadedLevel = UEditorLoadingAndSavingUtils::LoadMap(Asset.GetObjectPathString()))
			{
				GEditor->Exec(LoadedLevel, TEXT("MAP CHECK"));
			}
		}

		if (AssetClass->IsChildOf<AActor>())
		{
			Cast<AActor>(AssetPtr)->CheckForErrors();
		}

		// Compile blueprints as part of their validation
		if (AssetClass->IsChildOf<UBlueprint>())
		{
			FCompilerResultsLog CompilerLog;
			CompilerLog.SetSourcePath(Asset.GetObjectPathString());

			CompilerLog.BeginEvent(TEXT("Compile"));
			FKismetEditorUtilities::CompileBlueprint(
				Cast<UBlueprint>(AssetPtr),
				EBlueprintCompileOptions::SkipGarbageCollection | EBlueprintCompileOptions::SkipSave,
				&CompilerLog);
			CompilerLog.EndEvent();

			for (const auto& Message : CompilerLog.Messages)
			{
				if (Message->GetSeverity() != EMessageSeverity::Info)
				{
					AssetErrorStrings.Add(Message->ToText().ToString());
				}
			}
		}
	}

	// Run all asset types through asset validation system
	{
		TArray<FText> PackageErrors, PackageWarnings;
		const auto AssetResult =
			EditorValidationSubsystem
				.IsAssetValid(AssetPtr, OUT PackageErrors, OUT PackageWarnings, EDataValidationUsecase::Commandlet);

		if (AssetResult == EDataValidationResult::Invalid)
		{
			for (auto& PackageError : PackageErrors)
			{
				AssetErrorStrings.Add(PackageError.ToString());
			}
		}
	}

	// Add all map check errors that were reported during this assets validation.
	auto MapCheckMessages = MapCheckListing->GetFilteredMessages();
	for (const auto& MapCheckMessage : MapCheckMessages)
	{
		if (MapCheckMessage->GetSeverity() != EMessageSeverity::Info)
		{
			auto MapCheckMessageString = MapCheckMessage->ToText().ToString();
			if (MapCheckMessageString.Contains(TEXT("See the MapCheck log messages for details")))
			{
				// Ignore the "See the MapCheck log messages for details" message that is added for every actor
				// that fails the map check.
				continue;
			}
			AssetErrorStrings.Add(MapCheckMessageString);
		}
	}

	if (AssetErrorStrings.Num() > 0)
	{
		const FString PackageErrorString = FString::Join(AssetErrorStrings, LINE_TERMINATOR);
		OutReportObject->FJsonObject::SetStringField(Asset.PackageName.ToString(), PackageErrorString);

		++OutNumInvalidAssets;
	}

	// For world assets we loaded an entire map above; release it and collect garbage so leftover actors
	// (e.g. a reinstanced/duplicated WorldSettings) don't linger and get misreported by a later MAP CHECK.
	// See OUU::Editor::ValidateAssetList::Private::PurgeLoadedMap for details.
	if (const auto* AssetClass = Asset.GetClass(); AssetClass != nullptr && AssetClass->IsChildOf<UWorld>())
	{
		OUU::Editor::ValidateAssetList::Private::PurgeLoadedMap();
	}
}
