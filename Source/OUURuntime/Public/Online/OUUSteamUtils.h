// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUSteamUtils.generated.h"

UCLASS()
class OUURUNTIME_API UOUUSteamUtils : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static FString GetSteamAppIdFilename();

	// Returns true if the Steam DLC with the given AppID is installed/owned.
	// Always false when built without Steam support (WITH_STEAM=0) and in the editor (WITH_EDITOR).
	static bool IsDlcInstalled(int32 SteamDlcAppId);

	static bool WriteSteamAppIdToDisk(int32 SteamAppId);

	UFUNCTION(BlueprintCallable)
	static bool WriteSteamAppIdToDisk();
};
