// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SceneView.h"
#include "SceneProjectionLibrary.generated.h"

class UCameraComponent;
class APlayerController;

// #TODO-OpenUnrealUtilities Add overloads for arbitrary view target actors 
/**
 * Utility library that allows arbitrary projection from world to screen space and vice versa.
 */
UCLASS()
class OUURUNTIME_API UOUUSceneProjectionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * Get SceneViewProjectionData for a given camera
	 * @paramOUURequestQueue_TestRequest TargetCamera:
	 * @param Player: The player for this view projection. Required for Viewport and stereo settings. 
	 * @param OutProjectionData: Resulting projection data
	 * @returns if the operation was successful
	 * Copied from LocalPlayer::GetProjectionData(), which always uses the (cached) active ViewTarget
	 */
	static bool GetViewProjectionData(UCameraComponent* TargetCamera, APlayerController const* Player, FSceneViewProjectionData& OutProjectionData);

	/**
	 * Project a world location to screen space using a camera component as reference.
	 * As opposed to the APlayerController::ProjectWorldLocationToScreen() this function does not rely on the last rendered frame, but can predict
	 * arbitrary perspectives based on the current camera component settings.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Scene Projection")
	static bool ProjectWorldToScreen(UCameraComponent* TargetCamera, APlayerController const* Player, const FVector& WorldPosition, FVector2D& OutScreenPosition, bool bPlayerViewportRelative = true);

	/**
	* Deproject a screen space location to 3D world space using a camera component as reference.
	* As opposed to the APlayerController::DeprojectScreenPositionToWorld() this function does not rely on the last rendered frame, but can predict
	* arbitrary perspectives based on the current camera component settings.
	*/
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Scene Projection")
	static bool DeprojectScreenToWorld(UCameraComponent* TargetCamera, APlayerController const* Player, const FVector2D& ScreenPosition, FVector& OutWorldPosition, FVector& OutWorldDirection);
};
