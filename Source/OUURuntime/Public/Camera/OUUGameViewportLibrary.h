// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SceneView.h"

#include "OUUGameViewportLibrary.generated.h"

UCLASS()
class OUURUNTIME_API UOUUGameViewportLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Game Viewport")
	static void UpdateSplitscreenInfo(int32 ScreenIndex, int32 PlayerIndex, FVector2D Origin, FVector2D Size);
};
