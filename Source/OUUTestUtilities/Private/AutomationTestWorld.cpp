// Copyright (c) 2021 Jonas Reich

#pragma once

#include "AutomationTestWorld.h"

#if WITH_AUTOMATION_WORKER

#include "LogOpenUnrealUtilities.h"
#include "Engine/EngineTypes.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerState.h"
#include "Traits/IteratorTraits.h"
#include "UObject/CoreOnline.h"

FOUUAutomationTestWorld::~FOUUAutomationTestWorld()
{
	if (!ensureMsgf(!bHasWorld, TEXT("Undestroyed world found! You must always explicitly delete/cleanup automation test worlds that are not FScopedAutomationTestWorlds!!!")))
	{
		// Explicitly call FOUUAutomationTestWorld version of the virtual function
		// -> happens anyways because it's called in destructor
		FOUUAutomationTestWorld::DestroyWorld();
	}
}

void FOUUAutomationTestWorld::CreateWorld()
{
	CreateWorldImplementation();
}

FWorldContext& FOUUAutomationTestWorld::GetWorldContext() const
{
	return GEngine->GetWorldContextFromWorldChecked(World);
}

FTimerManager& FOUUAutomationTestWorld::GetTimerManager() const
{
	return World->GetTimerManager();
}

void FOUUAutomationTestWorld::BeginPlay()
{
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Could not send BeginPlay to invalid world!"));
		return;
	}
	World->InitializeActorsForPlay(URL);
	World->BeginPlay();
}

#define CHECK_INIT_GAME_CONDITION(Condition, ErrorString) \
if (Condition) { \
	UE_LOG(LogOpenUnrealUtilities, Error, TEXT("%s"), ToCStr(ErrorString)); \
	return false; \
}

bool FOUUAutomationTestWorld::InitializeGame()
{
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Could not InitiailizeGame invalid world!"));
		return false;
	}

	// Create and initialize game instance
	GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone(); // -> indiretly calls GameInstance->Init();

	auto* GameInstanceWorldContext = GameInstance->GetWorldContext();
	// This world is created as a temporary placeholder inside UGameInstance::InitializeStandalone()
	UWorld* GI_TempWorld = GameInstanceWorldContext->World();

	// Must destroy the temp world to prevent memory leaks!
	// We are supplying the new world that we are "travelling" to, in order to prevent GC of our test world. 
	GI_TempWorld->DestroyWorld(true, World);

	// Tell the game instance and world about each other.
	GameInstanceWorldContext->SetCurrentWorld(World);
	World->SetGameInstance(GameInstance);

	// Set game mode
	bool bIsGameModeSet = World->SetGameMode(URL);
	CHECK_INIT_GAME_CONDITION(!bIsGameModeSet, "Failed to set game mode");
	GameMode = World->GetAuthGameMode();

	// Debug error string required for many of the initialization functions on UWorld
	FString ErrorString;

	// Create a local player
	GameMode->PlayerStateClass = APlayerState::StaticClass();
	LocalPlayer = World->GetGameInstance()->CreateLocalPlayer(0, OUT ErrorString, false);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);
	CHECK_INIT_GAME_CONDITION(LocalPlayer == nullptr, "Failed to spawn LocalPlayer: returned nullptr");

	// Begin play for all actors
	BeginPlay();

	// Create a new unique net ID to spawn the local play actor = PlayerController
	TSharedPtr<const FUniqueNetId> UniqueNetId = GameInstance->GetPrimaryPlayerUniqueId();
	FUniqueNetIdRepl NetIdRepl = UniqueNetId;

	PlayerController = World->SpawnPlayActor(LocalPlayer, ENetRole::ROLE_Authority, URL, NetIdRepl, OUT ErrorString);
	CHECK_INIT_GAME_CONDITION(ErrorString.Len() > 0, ErrorString);
	CHECK_INIT_GAME_CONDITION(PlayerController == nullptr, "Failed to spawn PlayerController: returned nullptr");

	return true;
}
#undef CHECK_INIT_GAME_CONDITION

void FOUUAutomationTestWorld::DestroyWorld()
{
	DestroyWorldImplementation();
}

void FOUUAutomationTestWorld::CreateWorldImplementation()
{
	if (!ensureMsgf(!bHasWorld, TEXT("Undestroyed world found! You must always explicitly delete/cleanup automation test worlds that are not FScopedAutomationTestWorlds!!!")))
	{
		DestroyWorldImplementation();
	}

	FString NewWorldName = "OUUAutomationTestWorld";
	if (WorldName.Len() > 0)
	{
		NewWorldName += "_" + WorldName;
	}
	World = UWorld::CreateWorld(EWorldType::Game, true, *NewWorldName);
	auto& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
	WorldContext.SetCurrentWorld(World);
	World->GetWorldSettings()->DefaultGameMode = AGameModeBase::StaticClass();

	bHasWorld = true;
}

void FOUUAutomationTestWorld::DestroyWorldImplementation()
{
	// Prevent destroying world twice
	if (!bHasWorld)
		return;

	// Copied from UGameEngine::PreExit()
	{
		World->BeginTearingDown();

		// Cancel any pending connection to a server
		// commented out because it's not exposed, but it should not be required for test worlds atm
		// CancelPending(World);

		// Shut down any existing game connections
		GEngine->ShutdownWorldNetDriver(World);

		for (FActorIterator ActorIt(World); ActorIt; ++ActorIt)
		{
			ActorIt->RouteEndPlay(EEndPlayReason::Quit);
		}

		if (World->GetGameInstance() != nullptr)
		{
			World->GetGameInstance()->Shutdown();
		}

		World->FlushLevelStreaming(EFlushLevelStreamingType::Visibility);
		World->CleanupWorld();
	}

	World->DestroyWorld(true);
	GEngine->DestroyWorldContext(World);

	World = nullptr;
	GameInstance = nullptr;
	GameMode = nullptr;
	LocalPlayer = nullptr;
	PlayerController = nullptr;

	bHasWorld = false;
}

FOUUScopedAutomationTestWorld::FOUUScopedAutomationTestWorld(FString InWorldName) : FOUUAutomationTestWorld(InWorldName)
{
	CreateWorldImplementation();
}

FOUUScopedAutomationTestWorld::~FOUUScopedAutomationTestWorld()
{
	DestroyWorldImplementation();
}

void FOUUScopedAutomationTestWorld::CreateWorld()
{
	ensureMsgf(false, TEXT("CreateWorld must not be called on scoped automation worlds! Let RAII take care of world creation/destruction!"));
}

void FOUUScopedAutomationTestWorld::DestroyWorld()
{
	ensureMsgf(false, TEXT("DestroyWorld must not be called on scoped automation worlds! Let RAII take care of world creation/destruction!"));
}

#endif
