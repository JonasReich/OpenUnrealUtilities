// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GauntletTestController.h"
#include "Interfaces/OnlineSessionInterface.h"

#include "OUUMultiplayerTestController.generated.h"

enum class EFunctionalTestResult : uint8;
class AOUUMultiplayerFunctionalTest;

UCLASS()
class OUUTESTUTILITIES_API UOUUMultiplayerTestController : public UGauntletTestController
{
	GENERATED_BODY()
public:
	friend class AOUUMultiplayerFunctionalTest;

	/** @returns the single initialized local controller instance. */
	static UOUUMultiplayerTestController& Get();

	void NotifyServerPostSignalReplicated();
	// Notify the controller that a functional test started.
	void NotifyFunctionalTestStarted();
	void NotifyFunctionalTestEnded(EFunctionalTestResult TestResult, int32 TestIndex, int32 TotalNumTests);

	// - UGauntletTestController
	void OnInit() override;
	void OnPostMapChange(UWorld* World) override;
	void OnTick(float TimeDelta) override;
	// --

private:
	// --- SHARED
	IOnlineSessionPtr GetSessions() const;
	FUniqueNetIdPtr GetLocalNetID() const;
	void Heartbeat(const FString& Message);
	// ---- SERVER
	void OnCreateSessionComplete(FName, bool Success);
	void ServerOnPostLogin(AGameModeBase* GameModeBase, APlayerController* PlayerController);
	void ServerCheckRunFirstTest();
	void ServerRunNextFunctionalTest();
	// ---- CLIENT
	void ClientStartSessionSearch();
	void ClientSessionSearchComplete(bool Success);
	void ClientJoinSessionComplete(FName, EOnJoinSessionCompleteResult::Type Result);

private:
	// --- SHARED
	static UOUUMultiplayerTestController* Instance;
	FString TestRole;
	bool bIsServer = false;
	float TotalTickTime = 0.f;
	bool bUseSessionSearch = true;
	// --- SERVER
	bool bServerInitialized = false;
	bool bServerStartedFirstTest = false;
	int32 ServerNumFailedTests = 0;
	int32 ServerTotalNumTests = 0;
	int32 ServerNumFinishedTests = 0;
	int32 ServerNumSignalsReplicated = 0;
	TArray<TWeakObjectPtr<AOUUMultiplayerFunctionalTest>> ServerAllFunctionalTests;
	// --- CLIENT
	bool bClientSessionSearchStarted = false;
	TSharedPtr<FOnlineSessionSearch> ClientSessionSearch;
};

