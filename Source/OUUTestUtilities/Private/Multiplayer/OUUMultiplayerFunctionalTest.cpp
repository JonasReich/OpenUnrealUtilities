// Copyright (c) 2024 Jonas Reich & Contributors

#include "Multiplayer/OUUMultiplayerFunctionalTest.h"

#include "Animation/BoneChainRange.h"
#include "Multiplayer/OUUMultiplayerTestClientSignal.h"
#include "Multiplayer/OUUMultiplayerTestController.h"
#include "Net/UnrealNetwork.h"

AOUUMultiplayerFunctionalTest::AOUUMultiplayerFunctionalTest()
{
	bReplicates = true;
}

int32 AOUUMultiplayerFunctionalTest::AdvanceLocalSyncMarker()
{
	LastLocalSyncMarker++;
	if (HasAuthority())
	{
		// DO NOT acknowledge yet!
		// wait for client markers first...
		// LastServerAcknowledgedSyncMarker = LastLocalSyncMarker;
		UOUUMultiplayerTestController::Get().MarkHeartbeatActive(
			FString::Printf(TEXT("[SERVER] Sync Marker %i"), LastLocalSyncMarker));
	}
	else
	{
		int32 NumSignalActors = 0;
		for (auto Signal : TActorRange<AOUUMultiplayerTestClientSignal>(GetWorld()))
		{
			NumSignalActors++;
			// find the signal actor we can use to send RPCs to the server
			if (Signal->GetOwner())
			{
				Signal->Server_NotifySyncPointReached(this, LastLocalSyncMarker);
				return LastLocalSyncMarker;
			}
		}
		checkf(false, TEXT("None of the %i client signals have automous proxy role for this client"), NumSignalActors);
	}
	return LastLocalSyncMarker;
}

void AOUUMultiplayerFunctionalTest::ServerNotifyClientSyncMarkerReached(
	APlayerController* ClientPlayer,
	int32 SyncMarker)
{
	UOUUMultiplayerTestController::Get().MarkHeartbeatActive(
		FString::Printf(TEXT("[SERVER] Client Sync Marker %i"), SyncMarker));
	auto& UpdateEntry = ClientSyncMarkerLocations.FindOrAdd(ClientPlayer, -1);

	// check expected sync marker state. If this check fails, some sync marker was skipped.
	ensure(UpdateEntry == -1 || UpdateEntry == SyncMarker - 1 || UpdateEntry == SyncMarker);
	UpdateEntry = SyncMarker;

	// #TODO make parameter
	if (ClientSyncMarkerLocations.Num() < 2)
	{
		// not enough clients.
		// only relevant for first sync point until the map is filled with all players.
		return;
	}

	int32 LastClientAcknowledgedMarker = -1;
	for (auto& Entry : ClientSyncMarkerLocations)
	{
		if (LastClientAcknowledgedMarker == -1)
		{
			LastClientAcknowledgedMarker = Entry.Value;
		}
		else
		{
			LastClientAcknowledgedMarker = FMath::Min(Entry.Value, LastClientAcknowledgedMarker);
		}
		UE_LOG(LogTemp, Log, TEXT("%i"), Entry.Value);
	}
	UE_LOG(LogTemp, Log, TEXT("Last ACK Marker %i"), LastClientAcknowledgedMarker);

	if (LastClientAcknowledgedMarker == LastLocalSyncMarker)
	{
		LastServerAcknowledgedSyncMarker = LastLocalSyncMarker;
		UOUUMultiplayerTestController::Get().MarkHeartbeatActive(
			FString::Printf(TEXT("[SERVER] ACK Sync Marker %i"), LastServerAcknowledgedSyncMarker));

		OnSyncMarkerReached.Broadcast(LastClientAcknowledgedMarker);
	}
}
bool AOUUMultiplayerFunctionalTest::RunTest(const TArray<FString>& Params)
{
	UOUUMultiplayerTestController::Get().NotifyFunctionalTestStarted();
	return Super::RunTest(Params);
}

void AOUUMultiplayerFunctionalTest::FinishTest(EFunctionalTestResult TestResult, const FString& Message)
{
	ensureMsgf(
		HasAuthority(),
		TEXT("The FinishTest function should only ever be called on Authority! Use snyc point nodes for all "
			 "intermediate steps you want to ensure synchronicity."));
	UOUUMultiplayerTestController::Get().NotifyFunctionalTestEnded(TestResult);
	Super::FinishTest(TestResult, Message);
}

void AOUUMultiplayerFunctionalTest::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AOUUMultiplayerFunctionalTest, LastServerAcknowledgedSyncMarker);
}

void AOUUMultiplayerFunctionalTest::OnRep_ServerSyncMarker() const
{
	ensure(LastServerAcknowledgedSyncMarker == LastLocalSyncMarker);
	if (OnSyncMarkerReached.IsBound())
	{
		OnSyncMarkerReached.Broadcast(LastServerAcknowledgedSyncMarker);
	}
}

UOUUMultiplayerTestWaitForAll* UOUUMultiplayerTestWaitForAll::WaitForAll(AOUUMultiplayerFunctionalTest* InOwningTest)
{
	UOUUMultiplayerTestWaitForAll* Proxy = NewObject<UOUUMultiplayerTestWaitForAll>();
	Proxy->OwningTest = InOwningTest;
	return Proxy;
}

void UOUUMultiplayerTestWaitForAll::Activate()
{
	OwningTest->OnSyncMarkerReached.AddUObject(this, &UOUUMultiplayerTestWaitForAll::HandleSyncMarkerReached);
	MarkerIdx = OwningTest->AdvanceLocalSyncMarker();
}

void UOUUMultiplayerTestWaitForAll::HandleSyncMarkerReached(int32 Marker) const
{
	ensure(MarkerIdx == Marker);
	OwningTest->OnSyncMarkerReached.RemoveAll(this);
	OnComplete.Broadcast();
}
