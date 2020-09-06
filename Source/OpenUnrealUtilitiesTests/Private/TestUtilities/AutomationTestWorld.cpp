// Copyright (c) 2020 Jonas Reich

#pragma once

#include "TestUtilities/AutomationTestWorld.h"

#if WITH_AUTOMATION_WORKER

#include "OpenUnrealUtilitiesTestsModule.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystemTypes.h"
#include "Engine/EngineTypes.h"
#include "UObject/CoreOnline.h"

void FAutomationTestWorld::CreateWorld()
{
	World = UWorld::CreateWorld(EWorldType::Game, false);
	auto& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
}

FWorldContext& FAutomationTestWorld::GetWorldContext() const
{
	return GEngine->GetWorldContextFromWorldChecked(World);
}

void FAutomationTestWorld::BeginPlay()
{
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilitiesTests, Error, TEXT("Could not send BeginPlay to invalid world!"));
		return;
	}
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();
}

#define CHECK_INIT_GAME_CONDITION(Condition, ErrorString) \
if (Condition) { \
	UE_LOG(LogOpenUnrealUtilitiesTests, Error, TEXT("%s"), ToCStr(ErrorString)); \
	return false; \
}

bool FAutomationTestWorld::InitiailizeGame()
{
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilitiesTests, Error, TEXT("Could not InitiailizeGame invalid world!"));
		return false;
	}

	FString ErrorString;

	GameInstance = NewObject<UGameInstance>(GEngine);
	World->SetGameInstance(GameInstance);
	GameInstance->Init();
	bool bIsGameModeSet = World->SetGameMode(URL);
	CHECK_INIT_GAME_CONDITION(!bIsGameModeSet, "Failed to set game mode");
	GameMode = World->GetAuthGameMode();
	
	/*
	FString MapName = "";
	FString Options = "";
	GameMode->InitGame(MapName, Options, ErrorString);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);
	*/

	GameMode->PlayerStateClass = APlayerState::StaticClass();
	LocalPlayer = World->GetGameInstance()->CreateLocalPlayer(0, ErrorString, false);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);
	CHECK_INIT_GAME_CONDITION(LocalPlayer == nullptr, "Failed to spawn LocalPlayer: returned nullptr");

	BeginPlay();

	TSharedPtr<const FUniqueNetId> UniqueNetId = GameInstance->GetPrimaryPlayerUniqueId();
	FUniqueNetIdRepl NetIdRepl = UniqueNetId;

	PlayerController = World->SpawnPlayActor(LocalPlayer, ENetRole::ROLE_Authority, URL, NetIdRepl, ErrorString);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);
	CHECK_INIT_GAME_CONDITION(PlayerController == nullptr, "Failed to spawn PlayerController: returned nullptr");

	return true;
}
#undef CHECK_INIT_GAME_CONDITION

void FAutomationTestWorld::DestroyWorld()
{
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	World = nullptr;
	GameInstance = nullptr;
	GameMode = nullptr;
	LocalPlayer = nullptr;
	PlayerController = nullptr;
}

FScopedAutomationTestWorld::FScopedAutomationTestWorld()
{
	CreateWorld();
}

FScopedAutomationTestWorld::~FScopedAutomationTestWorld()
{
	DestroyWorld();
}

#endif