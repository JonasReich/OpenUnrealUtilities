// Copyright (c) 2022 Jonas Reich

#include "CoreMinimal.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Commandlets/Commandlet.h"
#include "Editor.h"
#include "Engine/Blueprint.h"
#include "Engine/Engine.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompilerModule.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

struct FOUUCompileBlueprintsCommandHelper
{
	// CommandLine Config Variables
	bool bResultsOnly = false;
	bool bSimpleAssetList = false;
	bool bCompileSkeletonOnly = false;
	bool bCookedOnly = false;
	bool bDirtyOnly = false;
	TArray<FString> IgnoreFolders;
	TArray<FString> WhitelistFiles;
	TArray<TPair<FString, TArray<FString>>> RequireAssetTags;
	TArray<TPair<FString, TArray<FString>>> ExcludeAssetTags;
	FName BlueprintBaseClassName = UBlueprint::StaticClass()->GetFName();

	// Variables to store overall results
	int32 TotalNumFailedLoads = 0;
	int32 TotalNumFatalIssues = 0;
	int32 TotalNumWarnings = 0;
	TArray<FString> AssetsWithErrorsOrWarnings;

	IKismetCompilerInterface* KismetBlueprintCompilerModule;
	TArray<FAssetData> BlueprintAssetList;
	double LastGCTime = 0;
	int32 CurrentBlueprintIndex = 0;

	FTimerHandle TickTimerHandle;

	FOUUCompileBlueprintsCommandHelper(const TArray<FString>& Args);

	~FOUUCompileBlueprintsCommandHelper();

	void QueueNextTick();

	void Tick();

	void InitCommandLine(const FString& Params);

	static void ParseTagPairs(const FString& FullTagString, TArray<TPair<FString, TArray<FString>>>& OutputAssetTags);

	void ParseIgnoreFolders(const FString& FullIgnoreFolderString);

	void ParseWhitelist(const FString& WhitelistFilePath);

	void BuildBlueprintAssetList();

	void TickImplementation();

	bool ShouldBuildAsset(FAssetData const& Asset) const;

	static bool CheckHasTagInList(
		FAssetData const& Asset,
		const TArray<TPair<FString, TArray<FString>>>& TagCollectionToCheck);

	bool CheckInWhitelist(FAssetData const& Asset) const;

	void CompileBlueprint(UBlueprint* Blueprint);

	void InitKismetBlueprintCompiler();

	void Shutdown();
};

TUniquePtr<FOUUCompileBlueprintsCommandHelper> GRecompileHelper;

FOUUCompileBlueprintsCommandHelper::FOUUCompileBlueprintsCommandHelper(const TArray<FString>& Args)
{
	FString ArgsLine = FString::Join(Args, TEXT(" "));
	if (ArgsLine.IsEmpty())
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("No arguments provided to ouu.CompileBlueprints command"))
	}

	InitCommandLine(ArgsLine);
	InitKismetBlueprintCompiler();

	BuildBlueprintAssetList();

	LastGCTime = FPlatformTime::Seconds();
	QueueNextTick();
}

FOUUCompileBlueprintsCommandHelper::~FOUUCompileBlueprintsCommandHelper()
{
	// Clear the timer, should the command be called again while the compile ticking is still happening.
	// This way, it should not break anything.
	GEditor->GetTimerManager()->ClearTimer(TickTimerHandle);
}

void FOUUCompileBlueprintsCommandHelper::QueueNextTick()
{
	FTimerDelegate TickDelegate;
	TickDelegate.BindRaw(this, &FOUUCompileBlueprintsCommandHelper::Tick);
	TickTimerHandle = GEditor->GetTimerManager()->SetTimerForNextTick(TickDelegate);
}

void FOUUCompileBlueprintsCommandHelper::Tick()
{
	if (CurrentBlueprintIndex < BlueprintAssetList.Num())
	{
		TickImplementation();
		CurrentBlueprintIndex++;
		QueueNextTick();
	}
	else
	{
		Shutdown();
	}
}

void FOUUCompileBlueprintsCommandHelper::InitCommandLine(const FString& Params)
{
	TArray<FString> Tokens;
	TArray<FString> Switches;
	TMap<FString, FString> SwitchParams;
	UCommandlet::ParseCommandLine(*Params, Tokens, Switches, SwitchParams);

	bResultsOnly = Switches.Contains(TEXT("ShowResultsOnly"));
	bDirtyOnly = Switches.Contains(TEXT("DirtyOnly"));
	bCookedOnly = Switches.Contains(TEXT("CookedOnly"));
	bSimpleAssetList = Switches.Contains(TEXT("SimpleAssetList"));

	RequireAssetTags.Empty();
	if (SwitchParams.Contains(TEXT("RequireTags")))
	{
		const FString& FullTagInfo = SwitchParams[TEXT("RequireTags")];
		ParseTagPairs(FullTagInfo, RequireAssetTags);
	}

	ExcludeAssetTags.Empty();
	if (SwitchParams.Contains(TEXT("ExcludeTags")))
	{
		const FString& FullTagInfo = SwitchParams[TEXT("ExcludeTags")];
		ParseTagPairs(FullTagInfo, ExcludeAssetTags);
	}

	IgnoreFolders.Empty();
	if (SwitchParams.Contains(TEXT("IgnoreFolder")))
	{
		const FString& AllIgnoreFolders = SwitchParams[TEXT("IgnoreFolder")];
		ParseIgnoreFolders(AllIgnoreFolders);
	}

	WhitelistFiles.Empty();
	if (SwitchParams.Contains(TEXT("WhitelistFile")))
	{
		const FString& WhitelistFullPath = SwitchParams[TEXT("WhitelistFile")];
		ParseWhitelist(WhitelistFullPath);
	}

	if (SwitchParams.Contains(TEXT("BlueprintBaseClass")))
	{
		BlueprintBaseClassName = *SwitchParams[TEXT("BlueprintBaseClass")];
	}
}

void FOUUCompileBlueprintsCommandHelper::ParseTagPairs(
	const FString& FullTagString,
	TArray<TPair<FString, TArray<FString>>>& OutputAssetTags)
{
	TArray<FString> AllTagPairs;
	FullTagString.ParseIntoArray(AllTagPairs, TEXT(";"));

	// Break All Tag Pairs into individual tags and values
	for (const FString& StringTagPair : AllTagPairs)
	{
		TArray<FString> ParsedTagPairs;
		StringTagPair.ParseIntoArray(ParsedTagPairs, TEXT(","));

		if (ParsedTagPairs.Num() > 0)
		{
			TArray<FString> TagValues;

			// Start AssetTagIndex at 1, as the first one is the key. We will be using index 0 in all adds as the
			// key
			for (int AssetTagIndex = 1; AssetTagIndex < ParsedTagPairs.Num(); ++AssetTagIndex)
			{
				TagValues.Add(ParsedTagPairs[AssetTagIndex]);
			}

			TPair<FString, TArray<FString>> NewPair(ParsedTagPairs[0], TagValues);
			OutputAssetTags.Add(NewPair);
		}
	}
}

void FOUUCompileBlueprintsCommandHelper::ParseIgnoreFolders(const FString& FullIgnoreFolderString)
{
	TArray<FString> ParsedIgnoreFolders;
	FullIgnoreFolderString.ParseIntoArray(ParsedIgnoreFolders, TEXT(","));

	for (const FString& IgnoreFolder : ParsedIgnoreFolders)
	{
		IgnoreFolders.Add(IgnoreFolder.TrimQuotes());
	}
}

void FOUUCompileBlueprintsCommandHelper::ParseWhitelist(const FString& WhitelistFilePath)
{
	const FString FilePath = FPaths::ProjectDir() + WhitelistFilePath;
	if (!FFileHelper::LoadANSITextFileToStrings(*FilePath, &IFileManager::Get(), WhitelistFiles))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to Load Whitelist File! : %s"), *FilePath);
	}
}

void FOUUCompileBlueprintsCommandHelper::BuildBlueprintAssetList()
{
	BlueprintAssetList.Empty();

	UE_LOG(LogOpenUnrealUtilities, Display, TEXT("Loading Asset Registry..."));
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(AssetRegistryConstants::ModuleName);
	auto& AssetRegistry = AssetRegistryModule.Get();
	if (AssetRegistry.IsLoadingAssets())
	{
		AssetRegistry.SearchAllAssets(/*bSynchronousSearch =*/true);
	}
	UE_LOG(LogOpenUnrealUtilities, Display, TEXT("Finished Loading Asset Registry."));

	UE_LOG(LogOpenUnrealUtilities, Display, TEXT("Gathering All Blueprints From Asset Registry..."));
	AssetRegistryModule.Get().GetAssetsByClass(BlueprintBaseClassName, BlueprintAssetList, true);
	UE_LOG(LogOpenUnrealUtilities, Display, TEXT("...found %i Blueprints"), BlueprintAssetList.Num());
}

void FOUUCompileBlueprintsCommandHelper::TickImplementation()
{
	FAssetData const& Asset = BlueprintAssetList[CurrentBlueprintIndex];
	if (!ShouldBuildAsset(Asset))
		return;

	const int32 NumAssets = BlueprintAssetList.Num();
	FString const AssetPath = Asset.ObjectPath.ToString();
	UE_LOG(
		LogOpenUnrealUtilities,
		Display,
		TEXT("Loading and Compiling [%.5i/%.5i]: '%s'..."),
		CurrentBlueprintIndex,
		NumAssets,
		*AssetPath);

	// Load with LOAD_NoWarn and LOAD_DisableCompileOnLoad as we are covering those explicitly with
	// CompileBlueprint errors.
	UBlueprint* LoadedBlueprint = Cast<UBlueprint>(StaticLoadObject(
		Asset.GetClass(),
		/*Outer =*/nullptr,
		*AssetPath,
		nullptr,
		LOAD_NoWarn | LOAD_DisableCompileOnLoad));
	if (LoadedBlueprint == nullptr)
	{
		++TotalNumFailedLoads;
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to Load : '%s'."), *AssetPath);
		return;
	}
	else
	{
		CompileBlueprint(LoadedBlueprint);
	}

	const double TimeNow = FPlatformTime::Seconds();

	if (TimeNow - LastGCTime >= 10.0)
	{
		GEngine->TrimMemory();
		LastGCTime = TimeNow;
	}
}

bool FOUUCompileBlueprintsCommandHelper::ShouldBuildAsset(FAssetData const& Asset) const
{
	bool bShouldBuild = true;

	if (bCookedOnly && Asset.GetClass() && !Asset.GetClass()->bCooked)
	{
		FString const AssetPath = Asset.ObjectPath.ToString();
		UE_LOG(LogOpenUnrealUtilities, Verbose, TEXT("Skipping Building %s: As is not cooked"), *AssetPath);
		bShouldBuild = false;
	}

	if (IgnoreFolders.Num() > 0)
	{
		for (const FString& IgnoreFolder : IgnoreFolders)
		{
			if (Asset.ObjectPath.ToString().StartsWith(IgnoreFolder))
			{
				FString const AssetPath = Asset.ObjectPath.ToString();
				UE_LOG(
					LogOpenUnrealUtilities,
					Verbose,
					TEXT("Skipping Building %s: As Object is in an Ignored Folder"),
					*AssetPath);
				bShouldBuild = false;
			}
		}
	}

	if ((ExcludeAssetTags.Num() > 0) && (CheckHasTagInList(Asset, ExcludeAssetTags)))
	{
		FString const AssetPath = Asset.ObjectPath.ToString();
		UE_LOG(LogOpenUnrealUtilities, Verbose, TEXT("Skipping Building %s: As has an excluded tag"), *AssetPath);
		bShouldBuild = false;
	}

	if ((RequireAssetTags.Num() > 0) && (!CheckHasTagInList(Asset, RequireAssetTags)))
	{
		FString const AssetPath = Asset.ObjectPath.ToString();
		UE_LOG(
			LogOpenUnrealUtilities,
			Verbose,
			TEXT("Skipping Building %s: As the asset is missing a required tag"),
			*AssetPath);
		bShouldBuild = false;
	}

	if ((WhitelistFiles.Num() > 0) && (!CheckInWhitelist(Asset)))
	{
		FString const AssetPath = Asset.ObjectPath.ToString();
		UE_LOG(
			LogOpenUnrealUtilities,
			Verbose,
			TEXT("Skipping Building %s: As the asset is not part of the whitelist"),
			*AssetPath);
		bShouldBuild = false;
	}

	if (bDirtyOnly)
	{
		const UPackage* AssetPackage = Asset.GetPackage();
		if ((AssetPackage == nullptr) || !AssetPackage->IsDirty())
		{
			FString const AssetPath = Asset.ObjectPath.ToString();
			UE_LOG(LogOpenUnrealUtilities, Verbose, TEXT("Skipping Building %s: As Package is not dirty"), *AssetPath);
			bShouldBuild = false;
		}
	}

	return bShouldBuild;
}

bool FOUUCompileBlueprintsCommandHelper::CheckHasTagInList(
	FAssetData const& Asset,
	const TArray<TPair<FString, TArray<FString>>>& TagCollectionToCheck)
{
	bool bContainedTag = false;

	for (const TPair<FString, TArray<FString>>& SingleTagAndValues : TagCollectionToCheck)
	{
		if (Asset.TagsAndValues.Contains(FName(*SingleTagAndValues.Key)))
		{
			const TArray<FString>& TagValuesToCheck = SingleTagAndValues.Value;
			if (TagValuesToCheck.Num() > 0)
			{
				for (const FString& IndividualValueToCheck : TagValuesToCheck)
				{
					if (Asset.TagsAndValues.ContainsKeyValue(FName(*SingleTagAndValues.Key), IndividualValueToCheck))
					{
						bContainedTag = true;
						break;
					}
				}
			}
			// if we don't have any values to check, just return true as the tag was included
			else
			{
				bContainedTag = true;
				break;
			}
		}
	}

	return bContainedTag;
}

bool FOUUCompileBlueprintsCommandHelper::CheckInWhitelist(FAssetData const& Asset) const
{
	bool bIsInWhitelist = false;

	const FString& AssetFilePath = Asset.ObjectPath.ToString();
	for (const FString& WhiteList : WhitelistFiles)
	{
		if (AssetFilePath == WhiteList)
		{
			bIsInWhitelist = true;
			break;
		}
	}

	return bIsInWhitelist;
}

void FOUUCompileBlueprintsCommandHelper::CompileBlueprint(UBlueprint* Blueprint)
{
	if (KismetBlueprintCompilerModule && Blueprint)
	{
		// Have to create a new MessageLog for each asset as the warning / error counts are cumulative
		FCompilerResultsLog MessageLog;
		// Need to prevent the Compiler Results Log from automatically outputting results if verbosity is too low
		if (bResultsOnly)
		{
			MessageLog.bSilentMode = true;
		}
		else
		{
			MessageLog.bAnnotateMentionedNodes = true;
		}
		MessageLog.SetSourcePath(Blueprint->GetPathName());
		MessageLog.BeginEvent(TEXT("Compile"));

		FKismetEditorUtilities::CompileBlueprint(
			Blueprint,
			EBlueprintCompileOptions::SkipGarbageCollection,
			&MessageLog);

		MessageLog.EndEvent();

		if ((MessageLog.NumErrors + MessageLog.NumWarnings) > 0)
		{
			AssetsWithErrorsOrWarnings.Add(Blueprint->GetPathName());

			TotalNumFatalIssues += MessageLog.NumErrors;
			TotalNumWarnings += MessageLog.NumWarnings;

			for (TSharedRef<class FTokenizedMessage>& Message : MessageLog.Messages)
			{
				FString MessageString = Message->ToText().ToString();
				if (Message->GetSeverity() == EMessageSeverity::Error)
				{
					UE_LOG(LogOpenUnrealUtilities, Error, TEXT("%s"), *MessageString);
				}
				else
				{
					UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("%s"), *MessageString);
				}
			}
		}
	}
}

void FOUUCompileBlueprintsCommandHelper::InitKismetBlueprintCompiler()
{
	UE_LOG(LogOpenUnrealUtilities, Display, TEXT("Loading Kismit Blueprint Compiler..."));
	// Get Kismet Compiler Setup. Static so that the expensive stuff only happens once per run.
	KismetBlueprintCompilerModule =
		&FModuleManager::LoadModuleChecked<IKismetCompilerInterface>(TEXT(KISMET_COMPILER_MODULENAME));
	UE_LOG(LogOpenUnrealUtilities, Display, TEXT("Finished Loading Kismit Blueprint Compiler..."));
}

void FOUUCompileBlueprintsCommandHelper::Shutdown()
{
	// results output
	UE_LOG(
		LogOpenUnrealUtilities,
		Display,
		TEXT(" \n \n \n"
			 "===================================================================================\n"
			 "Compiling Completed with %d errors and %d warnings and %d blueprints that failed to load.\n"
			 "===================================================================================\n \n \n"),
		TotalNumFatalIssues,
		TotalNumWarnings,
		TotalNumFailedLoads);

	// Assets with problems listing
	if (bSimpleAssetList && (AssetsWithErrorsOrWarnings.Num() > 0))
	{
		const FString AssetListString = FString::Join(AssetsWithErrorsOrWarnings, TEXT("\n"));
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT(" \n"
				 "===================================================================================\n"
				 "Assets With Errors or Warnings:\n"
				 "===================================================================================\n"
				 "%s\n"
				 "===================================================================================\n"
				 "End of Asset List\n"
				 "===================================================================================\n"),
			*AssetListString);
	}

	GRecompileHelper.Reset();
}

static FAutoConsoleCommand CompileBlueprintsCommand(
	TEXT("ouu.CompileBlueprints"),
	TEXT("Compile all Blueprints matching a certain pattern. "
		 "Same syntax as CompileAllBlueprints commandlet, but executed during editor time."),
	FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
		GRecompileHelper = MakeUnique<FOUUCompileBlueprintsCommandHelper>(Args);
	}));
