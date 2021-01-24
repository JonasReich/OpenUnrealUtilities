// Copyright (c) 2021 Jonas Reich

#pragma once

#include "TestUtilities/AutomationTestWorld.h"

#if WITH_AUTOMATION_WORKER

#include "OUUTestsModule.h"
#include "GameFramework/GameModeBase.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerState.h"
#include "Engine/EngineTypes.h"
#include "UObject/CoreOnline.h"

FAutomationTestWorld::~FAutomationTestWorld()
{
	if (!ensureMsgf(!bHasWorld, TEXT("Undestroyed world found! You must always explicitly delete/cleanup automation test worlds that are not FScopedAutomationTestWorlds!!!")))
	{
		DestroyWorld();
	}
}

void FAutomationTestWorld::CreateWorld()
{
	CreateWorldImplementation();
}

FWorldContext& FAutomationTestWorld::GetWorldContext() const
{
	return GEngine->GetWorldContextFromWorldChecked(World);
}

FTimerManager& FAutomationTestWorld::GetTimerManager() const
{
	return World->GetTimerManager();
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

bool FAutomationTestWorld::InitializeGame()
{
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilitiesTests, Error, TEXT("Could not InitiailizeGame invalid world!"));
		return false;
	}

	FString ErrorString;

	GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone(); // -> indiretly calls GameInstance->Init();
	GameInstance->GetWorldContext()->SetCurrentWorld(World);
	World->SetGameInstance(GameInstance);
	bool bIsGameModeSet = World->SetGameMode(URL);
	CHECK_INIT_GAME_CONDITION(!bIsGameModeSet, "Failed to set game mode");
	GameMode = World->GetAuthGameMode();
	
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
	DestroyWorldImplementation();
}

void FAutomationTestWorld::CreateWorldImplementation()
{
	if (!ensureMsgf(!bHasWorld, TEXT("Undestroyed world found! You must always explicitly delete/cleanup automation test worlds that are not FScopedAutomationTestWorlds!!!")))
	{
		DestroyWorldImplementation();
	}

	World = UWorld::CreateWorld(EWorldType::Game, false);
	auto& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);

	bHasWorld = true;
}

void FAutomationTestWorld::DestroyWorldImplementation()
{
	GEngine->DestroyWorldContext(World);
	World->DestroyWorld(false);

	World = nullptr;
	GameInstance = nullptr;
	GameMode = nullptr;
	LocalPlayer = nullptr;
	PlayerController = nullptr;

	bHasWorld = false;

	CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
}

FScopedAutomationTestWorld::FScopedAutomationTestWorld()
{
	CreateWorldImplementation();
}

FScopedAutomationTestWorld::~FScopedAutomationTestWorld()
{
	DestroyWorldImplementation();
}

void FScopedAutomationTestWorld::CreateWorld()
{
	ensureMsgf(false, TEXT("CreateWorld must not be called on scoped automation worlds! Let RAII take care of world creation/destruction!"));
}

void FScopedAutomationTestWorld::DestroyWorld()
{
	ensureMsgf(false, TEXT("DestroyWorld must not be called on scoped automation worlds! Let RAII take care of world creation/destruction!"));
}

#endif
