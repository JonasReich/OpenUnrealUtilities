// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SceneView.h"
#include "SceneProjectionLibrary.generated.h"

class UCameraComponent;
class APlayerController;

/**
 *
 */
UCLASS()
class OUURUNTIME_API USceneProjectionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Get SceneViewProjectionData for a given camera
	 * @param TargetCamera:
	 * @param Player: The player for this view projection. Required for Viewport and stereo settings. 
	 * @param OutProjectionData: Resulting projection data
	 * @returns if the operation was successful
	 * Copied from LocalPlayer::GetProjectionData(), which always uses the (cached) active ViewTarget
	 */
	static bool GetViewProjectionData(UCameraComponent* TargetCamera, APlayerController const* Player, FSceneViewProjectionData& OutProjectionData);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Scene Projection")
	static bool ProjectWorldToScreen(UCameraComponent* TargetCamera, APlayerController const* Player, const FVector& WorldPosition, FVector2D& OutScreenPosition, bool bPlayerViewportRelative = true);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Scene Projection")
	static bool DeprojectScreenToWorld(UCameraComponent* TargetCamera, APlayerController const* Player, const FVector2D& ScreenPosition, FVector& OutWorldPosition, FVector& OutWorldDirection);
};
