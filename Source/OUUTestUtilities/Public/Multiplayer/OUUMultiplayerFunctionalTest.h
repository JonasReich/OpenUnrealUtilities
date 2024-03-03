// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "FunctionalTest.h"

#include "OUUMultiplayerFunctionalTest.generated.h"

// Base class for multiplayer functional tests executed via Gauntlet.
// These tests do not work in editor since they require the gauntlet controller for coordination between agents.
UCLASS(Blueprintable)
class OUUTESTUTILITIES_API AOUUMultiplayerFunctionalTest : public AFunctionalTest
{
	GENERATED_BODY()
public:
	friend class UOUUMultiplayerTestWaitForAll;

	AOUUMultiplayerFunctionalTest();

	// For tracking where this test lies in the progression of all tests executed by the current gauntlet controller
	int32 TestIndex = INDEX_NONE;
	int32 TotalNumTests = INDEX_NONE;

	// Called on server + all clients when a sync point was reached.
	DECLARE_EVENT_OneParam(AOUUMultiplayerFunctionalTest, FOnSyncMarkerReached, int32);
	FOnSyncMarkerReached OnSyncMarkerReached;

	// Called on server + all clients after the test was started.
	// Use this as a start trigger in Blueprint graphs instead of the regular functional test start event.
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "Multiplayer Test Started")
	void K2_OnTestStarted();

	int32 AdvanceLocalSyncMarker();
	void ServerNotifyClientSyncMarkerReached(APlayerController* ClientPlayer, int32 SyncMarker);

	// - AFunctionalTest
	bool RunTest(const TArray<FString>& Params) override;
	void FinishTest(EFunctionalTestResult TestResult, const FString& Message) override;

	// - UObject
	void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	// --

private:
	UPROPERTY(Transient)
	TMap<APlayerController*, int32> ClientSyncMarkerLocations;

	UPROPERTY(Transient, Replicated, ReplicatedUsing = OnRep_ServerSyncMarker)
	int32 LastServerAcknowledgedSyncMarker = -1;

	int32 LastLocalSyncMarker = -1;
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnTestStarted();
	
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnTestEnded(EFunctionalTestResult TestResult, int32 InTestIndex, int32 InTotalNumTests);
	
	UFUNCTION()
	void OnRep_ServerSyncMarker() const;
};
