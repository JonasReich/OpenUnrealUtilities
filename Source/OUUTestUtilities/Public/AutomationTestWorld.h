// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Engine/Engine.h"

#if WITH_AUTOMATION_WORKER

/**
 * Helper struct that creates a new game world and world context for the purpose of using it in an automation test.
 * This is useful if you need to spawn actors for an automation test.
 * Implementation it modeled after the following engine examples:
 * - FAutomationAttachment::RunTest()
 * - FGameplayEffectsTest::RunTest()
 * - FTimerManagerTest::RunTest()
 */
struct OUUTESTUTILITIES_API FAutomationTestWorld
{
public:
	FAutomationTestWorld() = default;
	virtual ~FAutomationTestWorld();

	// Travel URL that will be used for game BeginPlay()
	FURL URL;

	// World pointer that will be set by CreateWorld()
	UWorld* World = nullptr;

	/**
	 * Create the world.
	 * Before this the World pointer will be null.
	 */
	virtual void CreateWorld(); 

	/**
	 * Get the world context for the world.
	 * Will crash if the world was not created yet or was already destroyed.
	 */
	FWorldContext& GetWorldContext() const;

	/** Get a reference to the world's timer manager */
	FTimerManager& GetTimerManager() const;

	/**
	 * Call this for actor initialization.
	 * This is a prerequisite to many game framework operations (e.g. spawning player controller, etc.).
	 * Requires prior world creation via CreateWorld().
	 */
	void BeginPlay();

	/** Pointers to game framework objects that will be set by InitiailizeGame() */
	UGameInstance* GameInstance = nullptr;
	AGameModeBase* GameMode = nullptr;
	ULocalPlayer* LocalPlayer = nullptr;
	APlayerController* PlayerController = nullptr;

	/**
	 * Initialize and spawn the most important game framework classes for tests that require gamemode, player controller, etc.
	 * Requires prior world creation via CreateWorld().
	 * You do not have to call BeginPlay() before this, as it will be called internally.
	 * @returns if everything went ok. This should always be the case, but you should use it as early exit condition for tests
	 * anyways to prevent crashes during test runs.
	 */
	bool InitializeGame();

	/**
	 * Destroy the world and world context.
	 * Afterwards none of the members may be used anymore to access the world or any of the game framework objects!
	 * This function must be called explicitly!
	 * Warning: This function forces garbage collection, so any stale UObjects created by tests will be destroyed!
	 */
	virtual void DestroyWorld();

protected:
	// Remember if we have a world that needs destruction
	bool bHasWorld = false;

	void CreateWorldImplementation();
	void DestroyWorldImplementation();
};

/**
 * Same as FAutomationTestWorld, but the world is automatically created during construction and
 * destroyed and cleaned up as soon as the FScopedAutomationWorld runs out of scope.
 * Note that in Automation Specs you usually want to use an FAutomationTestWorld instead because it can be
 * reused across scopes.
 * When using this Scoped alternative you should not call CreateWorld() and DestroyWorld anymore!
 */
struct OUUTESTUTILITIES_API FScopedAutomationTestWorld : public FAutomationTestWorld
{
public:
	using Super = FAutomationTestWorld;
	FScopedAutomationTestWorld();
	~FScopedAutomationTestWorld();

	// - FAutomationTestWorld
	void CreateWorld() override;
	void DestroyWorld() override;
	// --
};

#endif
