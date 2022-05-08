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
#include "Interfaces/IPluginManager.h"
#include "Interfaces/IProjectManager.h"
#include "Kismet2/CompilerResultsLog.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "KismetCompilerModule.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace OUU::Editor::CompileBlueprints
{
	struct FOUUCompileBlueprintsCommandHelper
	{
		// CommandLine Config Variables
		bool bResultsOnly = false;
		bool bSimpleAssetList = false;
		bool bCompileSkeletonOnly = false;
		bool bCookedOnly = false;
		bool bDirtyOnly = false;
		TArray<FString> IncludeFolders;
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

		FOUUCompileBlueprintsCommandHelper(FString ArgsLine);

		~FOUUCompileBlueprintsCommandHelper();

		void QueueNextTick();

		void Tick();

		void InitCommandLine(const FString& Params);

		static void ParseTagPairs(
			const FString& FullTagString,
			TArray<TPair<FString, TArray<FString>>>& OutputAssetTags);

		static void ParseFolders(const FString& FullFolderString, TArray<FString>& OutFolderList);

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

	FOUUCompileBlueprintsCommandHelper::FOUUCompileBlueprintsCommandHelper(FString ArgsLine)
	{
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

		IncludeFolders.Empty();
		if (SwitchParams.Contains(TEXT("IncludeFolders")))
		{
			const FString& AllIncludeFolders = SwitchParams[TEXT("IncludeFolders")];
			ParseFolders(AllIncludeFolders, IncludeFolders);
		}

		IgnoreFolders.Empty();
		if (SwitchParams.Contains(TEXT("IgnoreFolder")))
		{
			const FString& AllIgnoreFolders = SwitchParams[TEXT("IgnoreFolder")];
			ParseFolders(AllIgnoreFolders, IgnoreFolders);
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

	void FOUUCompileBlueprintsCommandHelper::ParseFolders(
		const FString& FullFolderString,
		TArray<FString>& OutFolderList)
	{
		TArray<FString> ParsedFolders;
		FullFolderString.ParseIntoArray(ParsedFolders, TEXT(","));

		for (const FString& Folder : ParsedFolders)
		{
			OutFolderList.Add(Folder.TrimQuotes());
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

		if (bShouldBuild && IncludeFolders.Num() > 0)
		{
			bShouldBuild = IncludeFolders.ContainsByPredicate([Asset](const FString& IncludeFolder) -> bool {
				return Asset.ObjectPath.ToString().StartsWith(IncludeFolder);
			});
			if (!bShouldBuild)
			{
				FString const AssetPath = Asset.ObjectPath.ToString();
				UE_LOG(
					LogOpenUnrealUtilities,
					Verbose,
					TEXT("Skipping Building %s: As Object is not in an Include Folder"),
					*AssetPath);
			}
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
				UE_LOG(
					LogOpenUnrealUtilities,
					Verbose,
					TEXT("Skipping Building %s: As Package is not dirty"),
					*AssetPath);
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
						if (Asset.TagsAndValues
								.ContainsKeyValue(FName(*SingleTagAndValues.Key), IndividualValueToCheck))
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

	FString GetIncludeFoldersArg(FString Mode)
	{
		TArray<FString> IncludeFoldersList;
		bool bIncludeProjectPlugins = false;
		bool bIncludeEnginePlugins = false;
		if (Mode == TEXT("Project"))
		{
			bIncludeProjectPlugins = true;
			IncludeFoldersList.Add(TEXT("/Game/"));
		}
		else if (Mode == TEXT("Engine"))
		{
			bIncludeEnginePlugins = true;
			IncludeFoldersList.Add(TEXT("/Engine/"));
		}
		else
		{
			// no filter
			return TEXT("");
		}

		TArray<TSharedRef<IPlugin>> AllContentPlugins = IPluginManager::Get().GetEnabledPluginsWithContent();
		for (TSharedRef<IPlugin> Plugin : AllContentPlugins)
		{
			const bool bIncludePlugin =
				(bIncludeProjectPlugins && Plugin->GetLoadedFrom() == EPluginLoadedFrom::Project)
				|| (bIncludeEnginePlugins && Plugin->GetLoadedFrom() == EPluginLoadedFrom::Engine);

			if (bIncludePlugin)
			{
				FString PluginContentPath = Plugin->GetMountedAssetPath();
				IncludeFoldersList.Add(PluginContentPath);
			}
		}

		return FString::Printf(TEXT("-IncludeFolders=%s"), *FString::Join(IncludeFoldersList, TEXT(",")));
	}

	static FAutoConsoleCommand CompileBlueprintsCommand_Preset(
		TEXT("ouu.CompileBlueprints.Preset"),
		TEXT("Compile all Blueprints matching the given pattern. Possible values: Project, Engine, All."),
		FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
			const FString Mode = Args.Num() > 0 ? Args[0] : "";
			const FString IncludeFoldersArg = GetIncludeFoldersArg(Mode);
			FString ArgsLine = FString::Printf(TEXT("-SimpleAssetList %s"), *IncludeFoldersArg);

			UE_LOG(
				LogOpenUnrealUtilities,
				Log,
				TEXT("Compiling all Blueprints with the following command line args: %s"),
				*ArgsLine);

			GRecompileHelper = MakeUnique<FOUUCompileBlueprintsCommandHelper>(ArgsLine);
		}));

	static FAutoConsoleCommand CompileBlueprintsCommand_Custom(
		TEXT("ouu.CompileBlueprints.Custom"),
		TEXT("Compile all Blueprints matching a certain pattern."),
		FConsoleCommandWithArgsDelegate::CreateStatic([](const TArray<FString>& Args) {
			FString ArgsLine = FString::Join(Args, TEXT(" "));
			if (ArgsLine.IsEmpty())
			{
				UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("No arguments provided to ouu.CompileBlueprints command"));
				return;
			}
			GRecompileHelper = MakeUnique<FOUUCompileBlueprintsCommandHelper>(ArgsLine);
		}));

} // namespace OUU::Editor::CompileBlueprints
