// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "FunctionalTest.h"
#include "Kismet/BlueprintAsyncActionBase.h"

#include "OUUMultiplayerFunctionalTest.generated.h"

UCLASS(Blueprintable)
class OUUTESTUTILITIES_API AOUUMultiplayerFunctionalTest : public AFunctionalTest
{
	GENERATED_BODY()
public:
	friend class UOUUMultiplayerTestWaitForAll;

	AOUUMultiplayerFunctionalTest();

	// Called on server + all clients if we detected that a sync point was reached.
	DECLARE_EVENT_OneParam(AOUUMultiplayerFunctionalTest, FOnSyncMarkerReached, int32);
	FOnSyncMarkerReached OnSyncMarkerReached;

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

	UPROPERTY(Transient, Replicated, ReplicatedUsing=OnRep_ServerSyncMarker)
	int32 LastServerAcknowledgedSyncMarker = -1;

	int32 LastLocalSyncMarker = -1;

	UFUNCTION()
	void OnRep_ServerSyncMarker() const;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOUUMultiplayerWaitForAll);

// This is the actual signal used in Blueprints of the functional tests
UCLASS(MinimalAPI)
class UOUUMultiplayerTestWaitForAll : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable)
	FOnOUUMultiplayerWaitForAll OnComplete;

	UFUNCTION(
		BlueprintCallable,
		meta = (BlueprintInternalUseOnly = "true", DefaultToSelf = "InOwningTest"),
		Category = "OpenUnrealUtilities|Testing")
	static UOUUMultiplayerTestWaitForAll* WaitForAll(AOUUMultiplayerFunctionalTest* InOwningTest);

	// - UBlueprintAsyncActionBase
	void Activate() override;
	// --
private:
	UPROPERTY()
	AOUUMultiplayerFunctionalTest* OwningTest = nullptr;
	int32 MarkerIdx = -1;

	void HandleSyncMarkerReached(int32 Marker) const;
};
