// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAssetConsoleVariables.h"

#include "JsonDAtaAsset/JsonDataAssetSubsystem.h"
#include "JsonDataAsset/JsonDataAssetGlobals.h"

namespace OUU::Runtime::JsonData::Private
{
	// Bool switches
	TAutoConsoleVariable<bool> CVar_SeparateSourceMountRoot(
		TEXT("ouu.JsonData.SeparateSourceMountRoot"),
		false,
		TEXT("If true, json data source files are placed in a separate packaged root directory compared to the "
			 "generated files. This is a more accurate representation of the files on disk (which are always in a "
			 "differently named directory), but is more cumbersome to navigate."));

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

	TAutoConsoleVariable<bool> CVar_UseFastNetSerialization(
		TEXT("ouu.JsonData.UseFastNetSerialization"),
		true,
		TEXT("If true, use fast net serialization for json data asset references. Drastically reduces network traffic, "
			 "but requires the list of json data asset files to be identical on all clients."));

	TAutoConsoleVariable<bool> CVar_IgnoreInvalidExtensions(
		TEXT("ouu.JsonData.IgnoreInvalidExtensions"),
		false,
		TEXT("If true, files with invalid extensions inside the Data/ folder will be ignored during 'import all "
			 "assets' calls."));

	// Config strings

	FString GDataSource_Uncooked = TEXT("Data/");
	FAutoConsoleVariableRef CVar_DataSource_Uncooked(
		TEXT("ouu.JsonData.SourceUncooked"),
		GDataSource_Uncooked,
		TEXT("Root relative path for uncooked json content. Must differ from cooked root and end in a slash!"),
		ECVF_ReadOnly);

	FString GDataSource_Cooked = TEXT("CookedData/");
	FAutoConsoleVariableRef CVar_DataSource_Cooked(
		TEXT("ouu.JsonData.SourceCooked"),
		GDataSource_Cooked,
		TEXT("Root relative path for cooked json content. Must differ from uncooked root and end in a slash!"),
		ECVF_ReadOnly);

	// Console commands

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
