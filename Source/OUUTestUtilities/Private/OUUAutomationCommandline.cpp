// Copyright (c) 2022 Jonas Reich

// NOTE: This file is in large parts copied from
// Engine\UE4\Source\Developer\AutomationController\Private\AutomationCommandline.cpp Some types and functions were
// renamed to avoid symbol name clashes with the engine files. The copy was made to make non-intrusive mods to test
// execution - primarily getting rid of initial test listings and adding options for additional test results exports.
// All mods compared to Epic's original code are marked with #OUU_MOD comments.

#include "CoreMinimal.h"

#include "AssetRegistryModule.h"
#include "AutomationControllerSettings.h"
#include "Containers/Ticker.h"
#include "IAutomationControllerModule.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreMisc.h"
#include "Misc/FilterCollection.h"
#include "Misc/Guid.h"
#include "Modules/ModuleManager.h"

DEFINE_LOG_CATEGORY_STATIC(LogOUUAutomationCommandLine, Log, All);

// #OUU_MOD Added OUU prefix
/** States for running the automation process */
enum class EOUUAutomationTestState : uint8
{
	Initializing,		//
	Idle,				// Automation process is not running
	FindWorkers,		// Find workers to run the tests
	RequestTests,		// Find the tests that can be run on the workers
	DoingRequestedWork, // Do whatever was requested from the commandline
	Complete			// The process is finished
};

// #OUU_MOD Added OUU prefix
enum class EOUUAutomationCommand : uint8
{
	None,				 // #OUU_MOD Added none entry - no action
	ListAllTests,		 // List all tests for the session
	RunCommandLineTests, // Run only tests that are listed on the commandline
	RunCheckpointTests,	 // Run only tests listed on the commandline with checkpoints in case of a crash.
	RunAll,				 // Run all the tests that are supported
	RunFilter,			 //
	Quit				 // quit the app when tests are done
};

// #OUU_MOD Added OUU prefix
class FOUUAutomationExecCmd : private FSelfRegisteringExec
{
public:
	static const float DefaultDelayTimer;
	static const float DefaultFindWorkersTimeout;

	FOUUAutomationExecCmd()
	{
		DelayTimer = DefaultDelayTimer;
		FindWorkersTimeout = DefaultFindWorkersTimeout;
	}

	void Init()
	{
		SessionID = FApp::GetSessionId();

		// Set state to FindWorkers to kick off the testing process
		AutomationTestState = EOUUAutomationTestState::Initializing;
		DelayTimer = DefaultDelayTimer;

		// Load the automation controller
		IAutomationControllerModule* AutomationControllerModule =
			&FModuleManager::LoadModuleChecked<IAutomationControllerModule>("AutomationController");
		AutomationController = AutomationControllerModule->GetAutomationController();

		AutomationController->Init();

		// TODO AUTOMATION Always use fullsize screenshots.
		const bool bFullSizeScreenshots = FParse::Param(FCommandLine::Get(), TEXT("FullSizeScreenshots"));
		const bool bSendAnalytics = FParse::Param(FCommandLine::Get(), TEXT("SendAutomationAnalytics"));

		// Register for the callback that tells us there are tests available
		if (!TestsRefreshedHandle.IsValid())
		{
			TestsRefreshedHandle = AutomationController->OnTestsRefreshed()
									   .AddRaw(this, &FOUUAutomationExecCmd::HandleRefreshTestCallback);
		}

		TickHandler =
			FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateRaw(this, &FOUUAutomationExecCmd::Tick));

		int32 NumTestLoops = 1;
		FParse::Value(FCommandLine::Get(), TEXT("TestLoops="), NumTestLoops);
		AutomationController->SetNumPasses(NumTestLoops);
		TestCount = 0;
		SetUpFilterMapping();
	}

	void SetUpFilterMapping()
	{
		FilterMaps.Empty();
		FilterMaps.Add("Engine", EAutomationTestFlags::EngineFilter);
		FilterMaps.Add("Smoke", EAutomationTestFlags::SmokeFilter);
		FilterMaps.Add("Stress", EAutomationTestFlags::StressFilter);
		FilterMaps.Add("Perf", EAutomationTestFlags::PerfFilter);
		FilterMaps.Add("Product", EAutomationTestFlags::ProductFilter);
	}

	void Shutdown()
	{
		IAutomationControllerModule* AutomationControllerModule =
			FModuleManager::GetModulePtr<IAutomationControllerModule>("AutomationController");
		if (AutomationControllerModule)
		{
			AutomationController = AutomationControllerModule->GetAutomationController();
			AutomationController->OnTestsRefreshed().RemoveAll(this);
		}

		FTicker::GetCoreTicker().RemoveTicker(TickHandler);
	}

	bool IsTestingComplete()
	{
		// If the automation controller is no longer processing and we've reached the final stage of testing
		if ((AutomationController->GetTestState() != EAutomationControllerModuleState::Running)
			&& (AutomationTestState == EOUUAutomationTestState::Complete) && (AutomationCommandQueue.Num() == 0))
		{
			// If an actual test was ran we then will let the user know how many of them were ran.
			if (TestCount > 0)
			{
				UE_LOG(
					LogOUUAutomationCommandLine,
					Display,
					TEXT("...Automation Test Queue Empty %d tests performed."),
					TestCount);
				TestCount = 0;
			}
			return true;
		}
		return false;
	}

	void GenerateTestNamesFromCommandLine(const TArray<FString>& AllTestNames, TArray<FString>& OutTestNames)
	{
		OutTestNames.Empty();

		// Split the argument names up on +
		TArray<FString> ArgumentNames;
		StringCommand.ParseIntoArray(ArgumentNames, TEXT("+"), true);

		TArray<FAutomatedTestFilter> Filters;

		// get our settings CDO where things are stored
		UAutomationControllerSettings* Settings =
			UAutomationControllerSettings::StaticClass()->GetDefaultObject<UAutomationControllerSettings>();

		// iterate through the arguments to build a filter list by doing the following -
		// 1) If argument is a filter (filter:system) then make sure we only filter-in tests that start with that filter
		// 2) If argument is a group then expand that group into multiple filters based on ini entries
		// 3) Otherwise just substring match (default behavior in 4.22 and earlier).
		for (int32 ArgumentIndex = 0; ArgumentIndex < ArgumentNames.Num(); ++ArgumentIndex)
		{
			const FString GroupPrefix = TEXT("Group:");
			const FString FilterPrefix = TEXT("Filter:");

			FString ArgumentName = ArgumentNames[ArgumentIndex];

			// if the argument is a filter (e.g. Filter:System) then create a filter that matches from the start
			if (ArgumentName.StartsWith(FilterPrefix))
			{
				FString FilterName = ArgumentName.RightChop(FilterPrefix.Len());

				if (FilterName.EndsWith(TEXT(".")) == false)
				{
					FilterName += TEXT(".");
				}

				Filters.Add(FAutomatedTestFilter(FilterName, true));
			}
			else if (ArgumentName.StartsWith(GroupPrefix))
			{
				// if the argument is a group (e.g. Group:Rendering) then seach our groups for one that matches
				FString GroupName = ArgumentName.RightChop(GroupPrefix.Len());

				bool FoundGroup = false;

				for (int32 i = 0; i < Settings->Groups.Num(); ++i)
				{
					FAutomatedTestGroup* GroupEntry = &(Settings->Groups[i]);
					if (GroupEntry && GroupEntry->Name == GroupName)
					{
						FoundGroup = true;
						// if found add all this groups filters to our current list
						if (GroupEntry->Filters.Num() > 0)
						{
							for (const FAutomatedTestFilter& GroupFilter : GroupEntry->Filters)
							{
								Filters.Add(GroupFilter);
							}
						}
						else
						{
							UE_LOG(
								LogOUUAutomationCommandLine,
								Warning,
								TEXT("Group %s contains no filters"),
								*GroupName);
						}
					}
				}

				if (!FoundGroup)
				{
					UE_LOG(LogOUUAutomationCommandLine, Warning, TEXT("No matching group named %s"), *GroupName);
				}
			}
			else
			{
				// old behavior of just string searching
				ArgumentName = ArgumentName.TrimStart().Replace(TEXT(" "), TEXT(""));

				bool bMatchFromStart = false;
				// bool bMatchFromEnd = false;

				if (ArgumentName.StartsWith("^"))
				{
					bMatchFromStart = true;
					ArgumentName.RightChopInline(1);
				}

				/*if (ArgumentName.EndsWith("$"))
				{
					bMatchFromEnd = true;
					ArgumentName.LeftChopInline(1);
				}*/

				// #agrant todo: restore in 4.26 when headers can be changed
				Filters.Add(FAutomatedTestFilter(ArgumentName, bMatchFromStart /*, bMatchFromEnd*/));
			}
		}

		for (int32 TestIndex = 0; TestIndex < AllTestNames.Num(); ++TestIndex)
		{
			FString TestNamesNoWhiteSpaces = AllTestNames[TestIndex].Replace(TEXT(" "), TEXT(""));

			for (const FAutomatedTestFilter& Filter : Filters)
			{
				// #agrant todo: remove in 4.26 when headers can be changed and we store this during parsing
				bool bMatchFromEnd = false;
				FString FilterString = Filter.Contains;
				if (FilterString.EndsWith("$"))
				{
					bMatchFromEnd = true;
					FilterString.LeftChopInline(1);
				}

				bool bNeedStartMatch = Filter.MatchFromStart;
				bool bNeedEndMatch = bMatchFromEnd;
				bool bMeetsMatch = true; // assume true

				// If we need to match at the start or end,
				if (bNeedStartMatch || bNeedEndMatch)
				{
					if (bNeedStartMatch)
					{
						bMeetsMatch = TestNamesNoWhiteSpaces.StartsWith(FilterString);
					}

					if (bNeedEndMatch && bMeetsMatch)
					{
						bMeetsMatch = TestNamesNoWhiteSpaces.EndsWith(FilterString);
					}
				}
				else
				{
					// match anywhere
					bMeetsMatch = TestNamesNoWhiteSpaces.Contains(FilterString);
				}

				if (bMeetsMatch)
				{
					OutTestNames.Add(AllTestNames[TestIndex]);
					TestCount++;
					break;
				}
			}
		}

		// If we have the TestsRun array set up and are using the same command as before, clear out already run tests.
		if (TestsRun.Num() > 0)
		{
			if (TestsRun[0] == StringCommand)
			{
				for (int i = 1; i < TestsRun.Num(); i++)
				{
					if (OutTestNames.Remove(TestsRun[i]))
					{
						UE_LOG(
							LogOUUAutomationCommandLine,
							Display,
							TEXT("Skipping %s due to Checkpoint."),
							*TestsRun[i]);
					}
				}
			}
			else
			{
				AutomationController->CleanUpCheckpointFile();
			}
		}
	}

	void FindWorkers(float DeltaTime)
	{
		DelayTimer -= DeltaTime;

		if (DelayTimer <= 0)
		{
			// Request the workers
			AutomationController->RequestAvailableWorkers(SessionID);
			AutomationTestState = EOUUAutomationTestState::RequestTests;
			FindWorkersTimeout = DefaultFindWorkersTimeout;
		}
	}

	void RequestTests(float DeltaTime)
	{
		FindWorkersTimeout -= DeltaTime;

		if (FindWorkersTimeout <= 0)
		{
			// Call the refresh callback manually
			HandleRefreshTestCallback();
		}
	}

	void HandleRefreshTestCallback()
	{
		TArray<FString> AllTestNames;

		if (AutomationController->GetNumDeviceClusters() == 0)
		{
			static double FirstWarningTime = FPlatformTime::Seconds();
			static int WarningCount = 0;

			double TimeWaiting = FPlatformTime::Seconds() - FirstWarningTime;

			// This can get called a number of times before a worker is ready, so be conservative in how often we warn
			if ((++WarningCount % 5) == 0 && TimeWaiting > 10.0)
			{
				UE_LOG(LogOUUAutomationCommandLine, Warning, TEXT("Can't find any workers! Searching again"));
			}
			else
			{
				UE_LOG(LogOUUAutomationCommandLine, Log, TEXT("Can't find any workers! Searching again"));
			}

			AutomationTestState = EOUUAutomationTestState::FindWorkers;
			return;
		}

		// We have found some workers
		// Create a filter to add to the automation controller, otherwise we don't get any reports
		AutomationController->SetFilter(MakeShareable(new AutomationFilterCollection()));
		AutomationController->SetVisibleTestsEnabled(true);
		AutomationController->GetEnabledTestNames(AllTestNames);

		// assume we won't run any tests
		bool bRunTests = false;

		if (AutomationCommand == EOUUAutomationCommand::ListAllTests)
		{
			UE_LOG(LogOUUAutomationCommandLine, Display, TEXT("Found %d Automation Tests"), AllTestNames.Num());
			for (const FString& TestName : AllTestNames)
			{
				UE_LOG(LogOUUAutomationCommandLine, Display, TEXT("\t%s"), *TestName);
			}

			// Set state to complete
			AutomationTestState = EOUUAutomationTestState::Complete;
		}
		else if (AutomationCommand == EOUUAutomationCommand::RunCommandLineTests)
		{
			TArray<FString> FilteredTestNames;
			GenerateTestNamesFromCommandLine(AllTestNames, FilteredTestNames);

			UE_LOG(
				LogOUUAutomationCommandLine,
				Display,
				TEXT("Found %d Automation Tests, based on '%s'."),
				FilteredTestNames.Num(),
				*StringCommand);

			// #OUU_MOD Commented out log listing of all tests. They take an unreasonably long time if you have many
			// automation tests (> 900)
			/*
			for (const FString& TestName : FilteredTestNames)
			{
				UE_LOG(LogOUUAutomationCommandLine, Display, TEXT("\t%s"), *TestName);
			}
			*/

			if (FilteredTestNames.Num())
			{
				AutomationController->StopTests();
				AutomationController->SetEnabledTests(FilteredTestNames);
				bRunTests = true;
			}
			else
			{
				AutomationTestState = EOUUAutomationTestState::Complete;
			}

			// Clear delegate to avoid re-running tests due to multiple delegates being added or when refreshing session
			// frontend The delegate will be readded in Init whenever a new command is executed
			AutomationController->OnTestsRefreshed().Remove(TestsRefreshedHandle);
			TestsRefreshedHandle.Reset();
		}
		else if (AutomationCommand == EOUUAutomationCommand::RunCheckpointTests)
		{
			TArray<FString> FilteredTestNames;
			GenerateTestNamesFromCommandLine(AllTestNames, FilteredTestNames);
			if (FilteredTestNames.Num())
			{
				AutomationController->StopTests();
				AutomationController->SetEnabledTests(FilteredTestNames);
				if (TestsRun.Num())
				{
					AutomationController->WriteLoadedCheckpointDataToFile();
				}
				else
				{
					AutomationController->WriteLineToCheckpointFile(StringCommand);
				}
				bRunTests = true;
			}
			else
			{
				AutomationTestState = EOUUAutomationTestState::Complete;
			}
		}
		else if (AutomationCommand == EOUUAutomationCommand::RunFilter)
		{
			if (FilterMaps.Contains(StringCommand))
			{
				UE_LOG(LogOUUAutomationCommandLine, Display, TEXT("Running %i Automation Tests"), AllTestNames.Num());
				AutomationController->SetEnabledTests(AllTestNames);
				bRunTests = true;
			}
			else
			{
				AutomationTestState = EOUUAutomationTestState::Complete;
				UE_LOG(
					LogOUUAutomationCommandLine,
					Display,
					TEXT("%s is not a valid flag to filter on! Valid options are: "),
					*StringCommand);
				TArray<FString> FlagNames;
				FilterMaps.GetKeys(FlagNames);
				for (int i = 0; i < FlagNames.Num(); i++)
				{
					UE_LOG(LogOUUAutomationCommandLine, Display, TEXT("\t%s"), *FlagNames[i]);
				}
			}
		}
		else if (AutomationCommand == EOUUAutomationCommand::RunAll)
		{
			bRunTests = true;
			TestCount = AllTestNames.Num();
		}

		if (bRunTests)
		{
			AutomationController->RunTests();

			// Set state to monitoring to check for test completion
			AutomationTestState = EOUUAutomationTestState::DoingRequestedWork;
		}
	}

	void MonitorTests()
	{
		if (AutomationController->GetTestState() != EAutomationControllerModuleState::Running)
		{
			// We have finished the testing, and results are available
			AutomationTestState = EOUUAutomationTestState::Complete;
		}
	}

	bool Tick(float DeltaTime)
	{
		QUICK_SCOPE_CYCLE_COUNTER(STAT_FAutomationExecCmd_Tick);

		// Update the automation controller to keep it running
		AutomationController->Tick();

		// Update the automation process
		switch (AutomationTestState)
		{
		case EOUUAutomationTestState::Initializing:
		{
			FAssetRegistryModule& AssetRegistryModule =
				FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
			if (AssetRegistryModule.Get().IsLoadingAssets() == false)
			{
				AutomationTestState = EOUUAutomationTestState::Idle;
			}

			break;
		}
		case EOUUAutomationTestState::FindWorkers:
		{
			// UE_LOG(LogOUUAutomationCommandLine, Log, TEXT("Finding Workers..."));

			FindWorkers(DeltaTime);
			break;
		}
		case EOUUAutomationTestState::RequestTests:
		{
			RequestTests(DeltaTime);
			break;
		}
		case EOUUAutomationTestState::DoingRequestedWork:
		{
			MonitorTests();
			break;
		}
		case EOUUAutomationTestState::Complete:
		case EOUUAutomationTestState::Idle:
		default:
		{
			// pop next command
			if (AutomationCommandQueue.Num())
			{
				AutomationCommand = AutomationCommandQueue[0];
				AutomationCommandQueue.RemoveAt(0);
				if (AutomationCommand == EOUUAutomationCommand::Quit)
				{
					if (AutomationCommandQueue.IsValidIndex(0))
					{
						// Add Quit back to the end of the array.
						AutomationCommandQueue.Add(EOUUAutomationCommand::Quit);
						break;
					}
				}
				AutomationTestState = EOUUAutomationTestState::FindWorkers;
			}

			// Only quit if Quit is the actual last element in the array.
			if (AutomationCommand == EOUUAutomationCommand::Quit)
			{
				if (!GIsCriticalError)
				{
					if (AutomationController->ReportsHaveErrors())
					{
						UE_LOG(
							LogOUUAutomationCommandLine,
							Display,
							TEXT("Setting GIsCriticalError due to test failures (will cause non-zero exit code)."));
						GIsCriticalError = AutomationController->ReportsHaveErrors();
					}
				}

				UE_LOG(LogOUUAutomationCommandLine, Log, TEXT("Forcing shutdown."));
				// some tools parse this.
				UE_LOG(
					LogOUUAutomationCommandLine,
					Display,
					TEXT("**** TEST COMPLETE. EXIT CODE: %d ****"),
					GIsCriticalError ? -1 : 0);
				FPlatformMisc::RequestExit(true);
				// We have finished the testing, and results are available
				AutomationTestState = EOUUAutomationTestState::Complete;
			}
			break;
		}
		}

		return !IsTestingComplete();
	}

	/** Console commands, see embeded usage statement **/
	virtual bool Exec(UWorld*, const TCHAR* Cmd, FOutputDevice& Ar) override
	{
		bool bHandled = false;
		// Track whether we have a flag we care about passing through.
		FString FlagToUse = "";

		// #OUU_MOD Commented out "hackiest hack"
		/*
		// Hackiest hack to ever hack a hack to get this test running.
		if (FParse::Command(&Cmd, TEXT("RunPerfTests")))
		{
			Cmd = TEXT("Automation RunFilter Perf");
		}
		else if (FParse::Command(&Cmd, TEXT("RunProductTests")))
		{
			Cmd = TEXT("Automation RunFilter Product");
		}
		*/

		// #OUU_MOD Changed command to "OUUAutomation" to avoid conflicts with native UE4 automation command line
		// figure out if we are handling this request
		if (FParse::Command(&Cmd, TEXT("OUUAutomation")))
		{
			StringCommand.Empty();

			TArray<FString> CommandList;
			StringCommand = Cmd;
			StringCommand.ParseIntoArray(CommandList, TEXT(";"), true);

			// assume we handle this
			Init();
			bHandled = true;

			for (int CommandIndex = 0; CommandIndex < CommandList.Num(); ++CommandIndex)
			{
				const TCHAR* TempCmd = *CommandList[CommandIndex];
				if (FParse::Command(&TempCmd, TEXT("StartRemoteSession")))
				{
					FString SessionString = TempCmd;
					if (!FGuid::Parse(SessionString, SessionID))
					{
						Ar.Logf(TEXT("%s is not a valid session guid!"), *SessionString);
						bHandled = false;
						break;
					}
				}
				else if (FParse::Command(&TempCmd, TEXT("List")))
				{
					AutomationCommandQueue.Add(EOUUAutomationCommand::ListAllTests);
				}
				else if (FParse::Command(&TempCmd, TEXT("RunTests")) || FParse::Command(&TempCmd, TEXT("RunTest")))
				{
					if (FParse::Command(&TempCmd, TEXT("Now")))
					{
						DelayTimer = 0.0f;
					}

					// only one of these should be used
					StringCommand = TempCmd;
					Ar.Logf(TEXT("Automation: RunTests='%s' Queued."), *StringCommand);
					AutomationCommandQueue.Add(EOUUAutomationCommand::RunCommandLineTests);
				}
				else if (FParse::Command(&TempCmd, TEXT("RunCheckpointedTests")))
				{
					StringCommand = TempCmd;
					Ar.Logf(TEXT("Running all tests with checkpoints matching substring: %s"), *StringCommand);
					AutomationCommandQueue.Add(EOUUAutomationCommand::RunCheckpointTests);
					TestsRun = AutomationController->GetCheckpointFileContents();
					AutomationController->CleanUpCheckpointFile();
				}
				else if (FParse::Command(&TempCmd, TEXT("SetMinimumPriority")))
				{
					StringCommand = TempCmd;
					Ar.Logf(TEXT("Setting minimum priority of cases to run to: %s"), *StringCommand);
					if (StringCommand.Contains(TEXT("Low")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::PriorityMask);
					}
					else if (StringCommand.Contains(TEXT("Medium")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::MediumPriority);
					}
					else if (StringCommand.Contains(TEXT("High")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::HighPriorityAndAbove);
					}
					else if (StringCommand.Contains(TEXT("Critical")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::ClientContext);
					}
					else if (StringCommand.Contains(TEXT("None")))
					{
						AutomationController->SetRequestedTestFlags(0);
					}
					else
					{
						Ar.Logf(
							TEXT("%s is not a valid priority!\nValid priorities are Critical, High, Medium, Low, None"),
							*StringCommand);
					}
				}
				else if (FParse::Command(&TempCmd, TEXT("SetPriority")))
				{
					StringCommand = TempCmd;
					Ar.Logf(TEXT("Setting explicit priority of cases to run to: %s"), *StringCommand);
					if (StringCommand.Contains(TEXT("Low")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::LowPriority);
					}
					else if (StringCommand.Contains(TEXT("Medium")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::MediumPriority);
					}
					else if (StringCommand.Contains(TEXT("High")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::HighPriority);
					}
					else if (StringCommand.Contains(TEXT("Critical")))
					{
						AutomationController->SetRequestedTestFlags(EAutomationTestFlags::CriticalPriority);
					}
					else if (StringCommand.Contains(TEXT("None")))
					{
						AutomationController->SetRequestedTestFlags(0);
					}

					else
					{
						Ar.Logf(
							TEXT("%s is not a valid priority!\nValid priorities are Critical, High, Medium, Low, None"),
							*StringCommand);
					}
				}
				else if (FParse::Command(&TempCmd, TEXT("RunFilter")))
				{
					FlagToUse = TempCmd;
					// only one of these should be used
					StringCommand = TempCmd;
					if (FilterMaps.Contains(FlagToUse))
					{
						AutomationController->SetRequestedTestFlags(FilterMaps[FlagToUse]);
						Ar.Logf(TEXT("Running all tests for filter: %s"), *FlagToUse);
					}
					AutomationCommandQueue.Add(EOUUAutomationCommand::RunFilter);
				}
				else if (FParse::Command(&TempCmd, TEXT("RunAll")))
				{
					AutomationCommandQueue.Add(EOUUAutomationCommand::RunAll);
					Ar.Logf(
						TEXT("Running all available automated tests for this program. NOTE: This may take a while."));
				}
				else if (FParse::Command(&TempCmd, TEXT("Quit")))
				{
					AutomationCommandQueue.Add(EOUUAutomationCommand::Quit);
					Ar.Logf(TEXT("Automation: Quit Command Queued."));
				}
				else
				{
					Ar.Logf(TEXT("Incorrect automation command syntax! Supported commands are: "));
					Ar.Logf(TEXT("\tAutomation StartRemoteSession <sessionid>"));
					Ar.Logf(TEXT("\tAutomation List"));
					Ar.Logf(TEXT("\tAutomation RunTests <test string>"));
					Ar.Logf(TEXT("\tAutomation RunAll "));
					Ar.Logf(TEXT("\tAutomation RunFilter <filter name>"));
					Ar.Logf(TEXT("\tAutomation Quit"));
					bHandled = false;
				}
			}
		}

		// Shutdown our service
		return bHandled;
	}

private:
	/** The automation controller running the tests */
	IAutomationControllerManagerPtr AutomationController;

	/** The current state of the automation process */
	EOUUAutomationTestState AutomationTestState;

	/** The priority flags we would like to run */
	EAutomationTestFlags::Type AutomationPriority;

	/** What work was requested */
	TArray<EOUUAutomationCommand> AutomationCommandQueue;

	// #OUU_MOD Added default value with new None case
	/** What work was requested */
	EOUUAutomationCommand AutomationCommand = EOUUAutomationCommand::None;

	/** Handle to Test Refresh delegate */
	FDelegateHandle TestsRefreshedHandle;

	/** Delay used before finding workers on game instances. Just to ensure they have started up */
	float DelayTimer;

	/** Timer Handle for giving up on workers */
	float FindWorkersTimeout;

	/** Holds the session ID */
	FGuid SessionID;

	// so we can release control of the app and just get ticked like all other systems
	FDelegateHandle TickHandler;

	// Extra commandline params
	FString StringCommand;

	// This is the numbers of tests that are found in the command line.
	int32 TestCount;

	// Dictionary that maps flag names to flag values.
	TMap<FString, int32> FilterMaps;

	// Test pass checkpoint backup file.
	FArchive* CheckpointFile;

	FString CheckpointCommand;

	TArray<FString> TestsRun;
};

const float FOUUAutomationExecCmd::DefaultDelayTimer = 5.0f;
const float FOUUAutomationExecCmd::DefaultFindWorkersTimeout = 30.0f;
static FOUUAutomationExecCmd AutomationExecCmd;

// #OUU_MOD Added OUU prefix
void OUU_EmptyLinkFunctionForStaticInitializationAutomationExecCmd()
{
	// This function exists to prevent the object file containing this test from
	// being excluded by the linker, because it has no publicly referenced symbols.
}
