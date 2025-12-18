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

	static bool WriteSteamAppIdToDisk(int32 SteamAppId);

	UFUNCTION(BlueprintCallable)
	static bool WriteSteamAppIdToDisk();
};
