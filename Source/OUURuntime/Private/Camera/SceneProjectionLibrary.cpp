// Copyright (c) 2023 Jonas Reich & Contributors

#include "Camera/SceneProjectionLibrary.h"

#include "Camera/CameraComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "IXRCamera.h"
#include "IXRTrackingSystem.h"
#include "Misc/EngineVersionComparison.h"
#include "SceneViewExtension.h"

bool UOUUSceneProjectionLibrary::GetViewProjectionData(
	UCameraComponent* TargetCamera,
	APlayerController const* Player,
	FSceneViewProjectionData& OutProjectionData)
{
	const ULocalPlayer* LocalPlayer = Player ? Player->GetLocalPlayer() : nullptr;
	if (!LocalPlayer || !LocalPlayer->ViewportClient)
	{
		return false;
	}

	FViewport* Viewport = LocalPlayer->ViewportClient->Viewport;
	EStereoscopicPass StereoPass = EStereoscopicPass::eSSP_FULL;

	const FIntPoint SizeXY = Viewport->GetSizeXY();
	if ((SizeXY.X == 0) || (SizeXY.Y == 0))
	{
		return false;
	}

	int32 X = FMath::TruncToInt(LocalPlayer->Origin.X * SizeXY.X);
	int32 Y = FMath::TruncToInt(LocalPlayer->Origin.Y * SizeXY.Y);

	X += Viewport->GetInitialPositionXY().X;
	Y += Viewport->GetInitialPositionXY().Y;

	uint32 SizeX = FMath::TruncToInt(LocalPlayer->Size.X * SizeXY.X);
	uint32 SizeY = FMath::TruncToInt(LocalPlayer->Size.Y * SizeXY.Y);

	const FIntRect UnconstrainedRectangle = FIntRect(X, Y, X + SizeX, Y + SizeY);

	OutProjectionData.SetViewRectangle(UnconstrainedRectangle);

	// Get the viewpoint.
	FMinimalViewInfo ViewInfo;
	TargetCamera->GetCameraView(0, ViewInfo);

	// If stereo rendering is enabled, update the size and offset appropriately for this pass
	const bool bNeedStereo = (StereoPass != EStereoscopicPass::eSSP_FULL) && GEngine->IsStereoscopic3D();
	const bool bIsHeadTrackingAllowed = GEngine->XRSystem.IsValid() && GEngine->XRSystem->IsHeadTrackingAllowed();
	// #TODO-OUU Adjust ViewIndex
	constexpr int32 ViewIndex = 0;
	if (bNeedStereo)
	{
		GEngine->StereoRenderingDevice->AdjustViewRect(ViewIndex, X, Y, SizeX, SizeY);
	}

	// scale distances for cull distance purposes by the ratio of our current FOV to the default FOV
	// LocalPlayer->PlayerController->LocalPlayerCachedLODDistanceFactor = ViewInfo.FOV / FMath::Max<float>(0.01f,
	// (LocalPlayer->PlayerController->PlayerCameraManager != NULL) ?
	// LocalPlayer->PlayerController->PlayerCameraManager->DefaultFOV : 90.f);

	FVector StereoViewLocation = ViewInfo.Location;
	if (bNeedStereo || bIsHeadTrackingAllowed)
	{
		const auto XRCamera = GEngine->XRSystem.IsValid() ? GEngine->XRSystem->GetXRCamera() : nullptr;
		if (XRCamera.IsValid())
		{
			const AActor* ViewTarget = LocalPlayer->PlayerController->GetViewTarget();
			const bool bHasActiveCamera = ViewTarget && ViewTarget->HasActiveCameraComponent();
			XRCamera->UseImplicitHMDPosition(bHasActiveCamera);
		}

		if (GEngine->StereoRenderingDevice.IsValid())
		{
			GEngine->StereoRenderingDevice->CalculateStereoViewOffset(
				ViewIndex,
				ViewInfo.Rotation,
				LocalPlayer->GetWorld()->GetWorldSettings()->WorldToMeters,
				StereoViewLocation);
		}
	}

	// Create the view matrix
	OutProjectionData.ViewOrigin = StereoViewLocation;
	OutProjectionData.ViewRotationMatrix = FInverseRotationMatrix(ViewInfo.Rotation)
		* FMatrix(FPlane(0, 0, 1, 0), FPlane(1, 0, 0, 0), FPlane(0, 1, 0, 0), FPlane(0, 0, 0, 1));

	// @todo viewext this use case needs to be revisited
	if (!bNeedStereo)
	{
		// Create the projection matrix (and possibly constrain the view rectangle)
		FMinimalViewInfo::CalculateProjectionMatrixGivenView(
			ViewInfo,
			LocalPlayer->AspectRatioAxisConstraint,
			Viewport,
			IN OUT OutProjectionData);

		const FSceneViewExtensionContext ViewExtensionContext(Viewport);
		for (auto& ViewExt : GEngine->ViewExtensions->GatherActiveExtensions(ViewExtensionContext))
		{
			ViewExt->SetupViewProjectionMatrix(OutProjectionData);
		};
	}
	else
	{
		// Let the stereoscopic rendering device handle creating its own projection matrix, as needed
#if UE_VERSION_OLDER_THAN(5, 0, 0)
		OutProjectionData.ProjectionMatrix = GEngine->StereoRenderingDevice->GetStereoProjectionMatrix(StereoPass);
#else
		OutProjectionData.ProjectionMatrix = GEngine->StereoRenderingDevice->GetStereoProjectionMatrix(ViewIndex);
#endif

		// calculate the out rect
		OutProjectionData.SetViewRectangle(FIntRect(X, Y, X + SizeX, Y + SizeY));
	}

	return true;
}

bool UOUUSceneProjectionLibrary::ProjectWorldToScreen(
	UCameraComponent* TargetCamera,
	APlayerController const* Player,
	const FVector& WorldPosition,
	FVector2D& OutScreenPosition,
	bool bPlayerViewportRelative /*= true*/)
{
	FSceneViewProjectionData ProjectionData;
	if (GetViewProjectionData(TargetCamera, Player, ProjectionData))
	{
		FMatrix const ViewProjectionMatrix = ProjectionData.ComputeViewProjectionMatrix();
		const bool bResult = FSceneView::ProjectWorldToScreen(
			WorldPosition,
			ProjectionData.GetConstrainedViewRect(),
			ViewProjectionMatrix,
			OutScreenPosition);

		if (bPlayerViewportRelative)
		{
			OutScreenPosition -= FVector2D(ProjectionData.GetConstrainedViewRect().Min);
		}
		return bResult;
	}

	OutScreenPosition = FVector2D::ZeroVector;
	return false;
}

bool UOUUSceneProjectionLibrary::DeprojectScreenToWorld(
	UCameraComponent* TargetCamera,
	APlayerController const* Player,
	const FVector2D& ScreenPosition,
	FVector& OutWorldPosition,
	FVector& OutWorldDirection)
{
	FSceneViewProjectionData ProjectionData;
	if (GetViewProjectionData(TargetCamera, Player, ProjectionData))
	{
		FMatrix const InvViewProjMatrix = ProjectionData.ComputeViewProjectionMatrix().InverseFast();
		FSceneView::DeprojectScreenToWorld(
			ScreenPosition,
			ProjectionData.GetConstrainedViewRect(),
			InvViewProjMatrix,
			/*out*/ OutWorldPosition,
			/*out*/ OutWorldDirection);
		return true;
	}

	OutWorldPosition = FVector::ZeroVector;
	OutWorldDirection = FVector::ZeroVector;
	return false;
}
