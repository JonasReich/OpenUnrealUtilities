// Copyright (c) 2024 Jonas Reich & Contributors

#include "Multiplayer/OUUMultiplayerTestWaitForAll.h"

#include "Multiplayer/OUUMultiplayerFunctionalTest.h"

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
