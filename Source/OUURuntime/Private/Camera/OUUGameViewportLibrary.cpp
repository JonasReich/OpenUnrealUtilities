// Copyright (c) 2023 Jonas Reich & Contributors

#include "Camera/OUUGameViewportLibrary.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

void UOUUGameViewportLibrary::UpdateSplitscreenInfo(
	int32 ScreenIndex,
	int32 PlayerIndex,
	FVector2D Origin,
	FVector2D Size)
{
	if (UGameViewportClient* GameViewportClient = GEngine->GameViewport)
	{
		const auto& SplitscreenInfo = GameViewportClient->SplitscreenInfo;
		if (SplitscreenInfo.IsValidIndex(ScreenIndex))
		{
			auto& PlayerData = GameViewportClient->SplitscreenInfo[ScreenIndex].PlayerData;
			if (PlayerData.IsValidIndex(PlayerIndex))
			{
				PlayerData[PlayerIndex].OriginX = Origin.X;
				PlayerData[PlayerIndex].OriginY = Origin.Y;
				PlayerData[PlayerIndex].SizeX = Size.X;
				PlayerData[PlayerIndex].SizeY = Size.Y;
			}
		}
	}
}
