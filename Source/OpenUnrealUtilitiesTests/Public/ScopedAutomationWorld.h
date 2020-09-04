// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

#if WITH_AUTOMATION_WORKER

/**
 * Helper struct that creates a new game world and world context for the purpose of using it in an automation test.
 * This is useful if you need to spawn actors for an automation test.
 * The world is automatically destroyed and cleaned up as soon as the FScopedAutomationWorld runs out of scope.
 * Implementation it modeled after the following engine examples:
 * - FAutomationAttachment::RunTest()
 * - FGameplayEffectsTest::RunTest()
 * - FTimerManagerTest::RunTest()
 */
struct OPENUNREALUTILITIESTESTS_API FScopedAutomationWorld
{
public:
	UWorld* World;
	FWorldContext& WorldContext;
	FURL URL;

	FScopedAutomationWorld() : 
		World (UWorld::CreateWorld(EWorldType::Game, false)),
		WorldContext (GEngine->CreateNewWorldContext(EWorldType::Game))
	{
		WorldContext.SetCurrentWorld(World);
	}

	~FScopedAutomationWorld()
	{
		GEngine->DestroyWorldContext(World);
		World->DestroyWorld(false);
	}

	/**
	 * Call this for actor initialization.
	 * This is a prerequisite to many game framework operations (e.g. spawning player controller, etc.)
	 */
	void BeginPlay();

	/** Pointers to game framework objects that will be set by InitiailizeGame() */
	UGameInstance* GameInstance;
	AGameModeBase* GameMode;
	ULocalPlayer* LocalPlayer;
	APlayerController* PlayerController;

	/**
	 * Initialize and spawn the most important game framework classes for tests that require gamemode, player controller, etc.
	 * @returns if everything went ok. This should always be the case, but you should use it as early exit condition for tests
	 * anyways to prevent crashes during test runs.
	 */
	bool InitiailizeGame();
};

#endif
