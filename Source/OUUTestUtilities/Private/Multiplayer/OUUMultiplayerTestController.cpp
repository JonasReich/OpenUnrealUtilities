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
#include "Multiplayer/OUUMultiplayerFunctionalTest.h"
#include "Multiplayer/OUUMultiplayerTestClientSignal.h"
#include "Online/OnlineSessionNames.h"
#include "OnlineSessionSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Traits/IteratorTraits.h"

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
	Heartbeat(FString::Printf(TEXT("Post signal replicated")));
}

void UOUUMultiplayerTestController::NotifyFunctionalTestStarted()
{
	Heartbeat(TEXT("Test started..."));
	if (GEngine->NetworkFailureEvent.IsBoundToObject(this) == false)
	{
		GEngine->NetworkFailureEvent
			.AddWeakLambda(this, [this](UWorld*, UNetDriver*, ENetworkFailure::Type Failure, const FString& Reason) {
				UE_LOG(
					LogOpenUnrealUtilities,
					Fatal,
					TEXT("Network failure (%s): %s"),
					ENetworkFailure::ToString(Failure),
					*Reason);
			});
	}
}

void UOUUMultiplayerTestController::NotifyFunctionalTestEnded(EFunctionalTestResult TestResult, int32 TestIndex, int32 TotalNumTests)
{
	if (TestIndex == TotalNumTests - 1)
	{
		// stop listening for network failures / disconnects.
		// even a successful client disconnect results in an error for me.
		GEngine->NetworkFailureEvent.RemoveAll(this);
	}

	Heartbeat(TEXT("Test ended..."));
	if (bIsServer)
	{
		ServerNumFinishedTests++;
		if (TestResult != EFunctionalTestResult::Succeeded)
		{
			ServerNumFailedTests++;
		}

		ServerRunNextFunctionalTest();
	}
}

void UOUUMultiplayerTestController::OnInit()
{
	check(Instance == nullptr);
	Instance = this;

	Super::OnInit();
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
	if (bIsServer && bUseSessionSearch)
	{
		if (bServerInitialized)
		{
			// Only do this once, otherwise we get into an infinite loop here.
			return;
		}
		bServerInitialized = true;

		Heartbeat(TEXT("Create session..."));
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
	else if (bUseSessionSearch)
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

void UOUUMultiplayerTestController::Heartbeat(const FString& Message)
{
	MarkHeartbeatActive(FString::Printf(TEXT("[%s] %s"), *TestRole, *Message));
}

void UOUUMultiplayerTestController::OnCreateSessionComplete(FName, bool Success)
{
	if (Success == false)
	{
		Heartbeat(TEXT("Failed to create session"));
		EndTest(1);
	}

	Heartbeat(TEXT("Enable listen server..."));
	GetWorld()->GetGameInstance()->EnableListenServer(true);
}

void UOUUMultiplayerTestController::ServerOnPostLogin(AGameModeBase* GameModeBase, APlayerController* PlayerController)
{
	auto* Signal = GetWorld()->SpawnActor<AOUUMultiplayerTestClientSignal>();
	check(Signal);
	Signal->SetOwner(PlayerController);
	Heartbeat(FString::Printf(TEXT("Post login (%s)"), *GetNameSafe(PlayerController)));
	ServerCheckRunFirstTest();
}

void UOUUMultiplayerTestController::ServerCheckRunFirstTest()
{
	// #TODO replace with parameter
	if (bServerStartedFirstTest == false && ServerNumSignalsReplicated == 2
		&& GetWorld()->GetGameState()->PlayerArray.Num() == 3)
	{
		Heartbeat(TEXT("Start condition fulfilled. Collecting tests..."));
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

		ServerRunNextFunctionalTest();
	}
}

void UOUUMultiplayerTestController::ServerRunNextFunctionalTest()
{
	bServerStartedFirstTest = true;
	if (ServerAllFunctionalTests.Num() == 0)
	{
		ensure(ServerNumFinishedTests == ServerTotalNumTests);
		Heartbeat(TEXT("All tests completed"));
		// Wait 3 more seconds for clients to disconnect.
		constexpr float EndTestDelay = 3.f;
		FTimerHandle TempHandle;
		GetWorld()->GetTimerManager().SetTimer(
			IN OUT TempHandle,
			FTimerDelegate::CreateLambda([this]() { EndTest(ServerNumFailedTests); }),
			EndTestDelay,
			false);
	}
	else
	{
		const auto NextTest = ServerAllFunctionalTests.Pop();
		check(NextTest.IsValid());
		Heartbeat(TEXT("Starting test..."));
		NextTest->TestIndex = ServerNumFinishedTests;
		NextTest->TotalNumTests = ServerTotalNumTests;
		IFunctionalTestingModule::Get().RunTestOnMap(NextTest->GetName(), false, false);
	}
}

void UOUUMultiplayerTestController::ClientStartSessionSearch()
{
	Heartbeat(TEXT("Start session search..."));

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
			Heartbeat(TEXT("Client travel..."));
			GetWorld()->GetFirstPlayerController()->ClientTravel(ConnectString, TRAVEL_Absolute);
		}
	}
}
