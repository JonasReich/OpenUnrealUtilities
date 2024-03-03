// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "OUUMultiplayerTestWaitForAll.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnOUUMultiplayerWaitForAll);

// Helper node to wait for on all sides (host & clients) for everyone to reach the same execution point.
// Only usable in the multiplayer functional tests.
// Triggering the node on either side triggers a mechanism wherein all parties agree which "sync marker" they last
// reached. OnComplete is called after server + clients completed this handshake (server initiated).
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
