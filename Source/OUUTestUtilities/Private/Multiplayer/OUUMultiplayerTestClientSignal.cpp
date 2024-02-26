// Copyright (c) 2024 Jonas Reich & Contributors

#include "Multiplayer/OUUMultiplayerTestClientSignal.h"

#include "GameFramework/PlayerController.h"
#include "Multiplayer/OUUMultiplayerFunctionalTest.h"
#include "Multiplayer/OUUMultiplayerTestController.h"

AOUUMultiplayerTestClientSignal::AOUUMultiplayerTestClientSignal()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	NetUpdateFrequency = 1.f;
}

void AOUUMultiplayerTestClientSignal::BeginPlay()
{
	Super::BeginPlay();
	if (HasAuthority())
	{
		// ???	
	}
	else if (GetOwner())
	{
		Server_NotifySignalOnClientSpawned();
	}
}

void AOUUMultiplayerTestClientSignal::Server_NotifySignalOnClientSpawned_Implementation()
{
	UOUUMultiplayerTestController::Get().NotifyServerPostSignalReplicated();
}

void AOUUMultiplayerTestClientSignal::Server_NotifySyncPointReached_Implementation(
	AOUUMultiplayerFunctionalTest* Test,
	int32 SyncPoint)
{
	if (!ensure(Test))
		return;

	auto* OwningPlayer = Cast<APlayerController> (GetOwner());
	check(OwningPlayer);
	Test->ServerNotifyClientSyncMarkerReached(OwningPlayer, SyncPoint);
}
