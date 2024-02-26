// Copyright (c) 2024 Jonas Reich & Contributors

#include "Multiplayer/OUUMultiplayerTestController.h"

#include "FunctionalTest.h"
#include "FunctionalTestingModule.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "LogOpenUnrealUtilities.h"
#include "Misc/AutomationTest.h"
#include "Multiplayer/OUUMultiplayerFunctionalTest.h"
#include "Multiplayer/OUUMultiplayerTestClientSignal.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Traits/IteratorTraits.h"

UE_DISABLE_OPTIMIZATION

namespace OUU::TestUtilities
{
	const FString GTestRole_Server = TEXT("Server");
	const FString GTestRole_Client = TEXT("Client");
} // namespace OUU::TestUtilities

UOUUMultiplayerTestController* UOUUMultiplayerTestController::Instance = nullptr;

UOUUMultiplayerTestController& UOUUMultiplayerTestController::Get()
{
	return *Instance;
}

void UOUUMultiplayerTestController::NotifyServerPostSignalReplicated()
{
	ServerNumSignalsReplicated++;
	MarkHeartbeatActive(FString::Printf(TEXT("[SERVER] post signal replicated")));
}

void UOUUMultiplayerTestController::NotifyFunctionalTestStarted()
{
	MarkHeartbeatActive(TEXT("[SERVER] test started..."));
			// #TODO start listening for disconnects?
	/*
	GEngine->NetworkFailureEvent.AddLambda(
		[this](UWorld*, UNetDriver*, ENetworkFailure::Type Failure, const FString& Reason) {
			UE_LOG(
				LogOpenUnrealUtilities,
				Fatal,
				TEXT("Network failure (%s): %s"),
				ENetworkFailure::ToString(Failure),
				*Reason);
		});
	*/
}

void UOUUMultiplayerTestController::NotifyFunctionalTestEnded(EFunctionalTestResult TestResult)
{
	MarkHeartbeatActive(TEXT("[SERVER] test ended..."));
	// #TODO stop listening for disconnects?
	ServerNumFinishedTests++;
	if (TestResult != EFunctionalTestResult::Succeeded)
	{
		ServerNumFailedTests++;
	}

	ServerRunNextFunctionalTest();
}

void UOUUMultiplayerTestController::OnInit()
{
	check(Instance == nullptr);
	Instance = this;

	Super::OnInit();
	FString TestRole;
	FParse::Value(FCommandLine::Get(), TEXT("OUUMPTestRole="), OUT TestRole);
	UE_LOG(LogOpenUnrealUtilities, Log, TEXT("OUUMPTestRole=%s"), *TestRole);
	bIsServer = TestRole == OUU::TestUtilities::GTestRole_Server;
	if (bIsServer)
	{
		FGameModeEvents::GameModePostLoginEvent.AddUObject(this, &UOUUMultiplayerTestController::ServerOnPostLogin);
	}
}

void UOUUMultiplayerTestController::OnPostMapChange(UWorld* World)
{
	if (bIsServer)
	{
		if (bServerInitialized)
		{
			// Only do this once, otherwise we get into an infinite loop here.
			return;
		}
		bServerInitialized = true;

		MarkHeartbeatActive(TEXT("[SERVER] create session..."));
		FOnlineSessionSettings Settings;
		Settings.NumPublicConnections = 2;
		Settings.bShouldAdvertise = true;
		Settings.bAllowJoinInProgress = true;
		Settings.bIsLANMatch = true;
		Settings.bUsesPresence = true;
		Settings.bAllowJoinViaPresence = true;

		GetSessions()->AddOnCreateSessionCompleteDelegate_Handle(FOnCreateSessionCompleteDelegate::CreateUObject(
			this,
			&UOUUMultiplayerTestController::OnCreateSessionComplete));
		GetSessions()->CreateSession(*GetLocalNetID(), NAME_GameSession, Settings);
	}
}

void UOUUMultiplayerTestController::OnTick(float TimeDelta)
{
	Super::OnTick(TimeDelta);
	TotalTickTime += TimeDelta;

	if (bIsServer)
	{
		ServerCheckRunFirstTest();
	}
	else
	{
		// #TODO is there a better way to delay client session search?
		constexpr float WaitUntilSessionSearch = 5.f;
		if (TotalTickTime > WaitUntilSessionSearch && bClientSessionSearchStarted == false)
		{
			bClientSessionSearchStarted = true;
			ClientStartSessionSearch();
		}
	}
}

IOnlineSessionPtr UOUUMultiplayerTestController::GetSessions() const
{
	auto* OnlineSubsystem = Online::GetSubsystem(GetWorld());
	auto Sessions = OnlineSubsystem->GetSessionInterface();
	return Sessions;
}

FUniqueNetIdPtr UOUUMultiplayerTestController::GetLocalNetID() const
{
	const auto* Player = UGameplayStatics::GetPlayerState(GetWorld(), 0);
	auto UniqueNetID = Player->GetUniqueId().GetUniqueNetId();
	return UniqueNetID;
}

void UOUUMultiplayerTestController::OnCreateSessionComplete(FName, bool Success)
{
	if (Success == false)
	{
		MarkHeartbeatActive(TEXT("[SERVER] failed to create session"));
		EndTest(1);
	}

	// CRASH???
	// as listen server?
	MarkHeartbeatActive(TEXT("[SERVER] enable listen server..."));
	GetWorld()->GetGameInstance()->EnableListenServer(true);

	MarkHeartbeatActive(TEXT("[SERVER] Collecting tests..."));
	for (auto* FTest : TActorRange<AFunctionalTest>(GetWorld()))
	{
		if (auto* MultiplayerFTest = Cast<AOUUMultiplayerFunctionalTest>(FTest))
		{
			ServerAllFunctionalTests.Add(MultiplayerFTest);
		}
		else
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Fatal,
				TEXT("Invalid functional test %s on map %s is not a multiplayer test"),
				*GetCurrentMap(),
				*GetNameSafe(FTest));
		}
	}

	ServerTotalNumTests = ServerAllFunctionalTests.Num();
}

void UOUUMultiplayerTestController::ServerOnPostLogin(AGameModeBase* GameModeBase, APlayerController* PlayerController)
{
	auto* Signal = GetWorld()->SpawnActor<AOUUMultiplayerTestClientSignal>();
	check(Signal);
	Signal->SetOwner(PlayerController);
	MarkHeartbeatActive(FString::Printf(TEXT("[SERVER] post login (%s)"), *GetNameSafe(PlayerController)));
}

void UOUUMultiplayerTestController::ServerCheckRunFirstTest()
{
	// #TODO replace with parameter
	if (bServerStartedFirstTest == false && ServerNumSignalsReplicated == 2 && GetWorld()->GetGameState()->PlayerArray.Num() == 3)
	{
		ServerRunNextFunctionalTest();
	}
}

void UOUUMultiplayerTestController::ServerRunNextFunctionalTest()
{
	bServerStartedFirstTest = true;
	if (ServerAllFunctionalTests.Num() == 0)
	{
		ensure(ServerNumFinishedTests == ServerTotalNumTests);
		MarkHeartbeatActive(TEXT("[SERVER] all tests completed"));
		EndTest(ServerNumFailedTests);
	}
	else
	{
		const auto NextTest = ServerAllFunctionalTests.Pop();
		check(NextTest.IsValid());
		MarkHeartbeatActive(TEXT("[SERVER] starting test..."));
		IFunctionalTestingModule::Get().RunTestOnMap(NextTest->GetName(), false, false);
	}
}

void UOUUMultiplayerTestController::OnFunctionalTestEnd(FAutomationTestBase* AutomationTest)
{
	MarkHeartbeatActive(FString(TEXT("test completed: ")) + AutomationTest->GetTestName());
	if (AutomationTest->GetLastExecutionSuccessState() == false)
	{
		ServerNumFailedTests++;
	}
}

void UOUUMultiplayerTestController::ClientStartSessionSearch()
{
	MarkHeartbeatActive(TEXT("[CLIENT] find sessions..."));

	ClientSessionSearch = MakeShared<FOnlineSessionSearch>();
	ClientSessionSearch->MaxSearchResults = 1;
	ClientSessionSearch->bIsLanQuery = true;
	ClientSessionSearch->QuerySettings.Set(SEARCH_PRESENCE, true, EOnlineComparisonOp::Equals);

	GetSessions()->AddOnFindSessionsCompleteDelegate_Handle(FOnFindSessionsCompleteDelegate::CreateUObject(
		this,
		&UOUUMultiplayerTestController::ClientSessionSearchComplete));
	GetSessions()->FindSessions(*GetLocalNetID(), ClientSessionSearch.ToSharedRef());
}

void UOUUMultiplayerTestController::ClientSessionSearchComplete(bool Success)
{
	GetSessions()->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UOUUMultiplayerTestController::ClientJoinSessionComplete));
	check(ClientSessionSearch->SearchResults.Num() > 0);
	GetSessions()->JoinSession(*GetLocalNetID(), NAME_GameSession, ClientSessionSearch->SearchResults[0]);
}

void UOUUMultiplayerTestController::ClientJoinSessionComplete(FName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result != EOnJoinSessionCompleteResult::Success)
	{
		EndTest(1);
	}
	else
	{
		// Client travel to the server
		FString ConnectString;
		if (GetSessions()->GetResolvedConnectString(NAME_GameSession, ConnectString))
		{
			UE_LOG_ONLINE_SESSION(Log, TEXT("Join session: traveling to %s"), *ConnectString);
			MarkHeartbeatActive(TEXT("[CLIENT] travel..."));
			GetWorld()->GetFirstPlayerController()->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
}

UE_ENABLE_OPTIMIZATION
