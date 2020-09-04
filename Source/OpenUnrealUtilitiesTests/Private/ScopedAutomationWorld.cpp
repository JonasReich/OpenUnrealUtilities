// Copyright (c) 2020 Jonas Reich

#pragma once

#include "ScopedAutomationWorld.h"
#include "OpenUnrealUtilitiesTests.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/OnlineReplStructs.h"
#include "OnlineSubsystemTypes.h"
#include "Engine/EngineTypes.h"

#if WITH_AUTOMATION_WORKER

void FScopedAutomationWorld::BeginPlay()
{
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();
}

#define CHECK_INIT_GAME_CONDITION(Condition, ErrorString) \
if (Condition) { \
	UE_LOG(LogOpenUnrealUtilitiesTests, Error, TEXT("%s"), ToCStr(ErrorString)); \
	return false; \
}

bool FScopedAutomationWorld::InitiailizeGame()
{
	FString ErrorString;

	GameInstance = NewObject<UGameInstance>(GEngine);
	World->SetGameInstance(GameInstance);
	bool bIsGameModeSet = World->SetGameMode(URL);
	CHECK_INIT_GAME_CONDITION(!bIsGameModeSet, "Failed to set game mode");
	GameMode = World->GetAuthGameMode();
	
	FString MapName = "";
	FString Options = "";
	GameMode->InitGame(MapName, Options, ErrorString);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);

	GameMode->PlayerStateClass = APlayerState::StaticClass();
	LocalPlayer = World->GetGameInstance()->CreateLocalPlayer(0, ErrorString, false);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);
	CHECK_INIT_GAME_CONDITION(LocalPlayer == nullptr, "Failed to spawn LocalPlayer: returned nullptr");

	TSharedPtr<FUniqueNetId> UniqueNetId = MakeShared<FUniqueNetIdString>();
	FUniqueNetIdRepl NetIdRepl = UniqueNetId;

	PlayerController = World->SpawnPlayActor(LocalPlayer, ENetRole::ROLE_Authority, URL, NetIdRepl, ErrorString);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);
	CHECK_INIT_GAME_CONDITION(PlayerController == nullptr, "Failed to spawn PlayerController: returned nullptr");

	return true;
}

#undef CHECK_INIT_GAME_CONDITION

#endif
