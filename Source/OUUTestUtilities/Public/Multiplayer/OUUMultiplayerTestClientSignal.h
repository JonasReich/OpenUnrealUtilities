// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Info.h"

#include "OUUMultiplayerTestClientSignal.generated.h"

class AOUUMultiplayerFunctionalTest;

// This class is used to send signals from clients to the server.
// With 2+ clients we need one actor for each client where they can be the NetOwner, so they can send RPCs. 
UCLASS(BlueprintType)
class AOUUMultiplayerTestClientSignal : public AInfo
{
	GENERATED_BODY()
public:
	friend class AOUUMultiplayerFunctionalTest;

	AOUUMultiplayerTestClientSignal();

	// - AActor
	void BeginPlay() override;
	// --
	
private:
	UFUNCTION(Server, Reliable)
	void Server_NotifySyncPointReached(AOUUMultiplayerFunctionalTest* Test, int32 SyncPoint);

	UFUNCTION(Server, Reliable)
	void Server_NotifySignalOnClientSpawned();
};
