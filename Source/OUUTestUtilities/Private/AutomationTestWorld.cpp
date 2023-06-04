// Copyright (c) 2023 Jonas Reich & Contributors

#include "AutomationTestWorld.h"

#if WITH_AUTOMATION_WORKER

	#include "Misc/EngineVersionComparison.h"
	#include "Engine/EngineTypes.h"
	#include "Engine/GameInstance.h"
	#include "Engine/LocalPlayer.h"
	#include "Engine/World.h"
	#include "GameFramework/GameModeBase.h"
	#include "GameFramework/PlayerState.h"
	#include "LogOpenUnrealUtilities.h"
	#include "Traits/IteratorTraits.h"
	#include "GameMapsSettings.h"

	#if UE_VERSION_OLDER_THAN(5, 0, 0)
		#include "UObject/CoreOnline.h"
	#else
		#include "Online/CoreOnline.h"
	#endif

FOUUAutomationTestWorld::FOUUAutomationTestWorld(FString InWorldName) :
	URL(TEXT("/OpenUnrealUtilities/Runtime/EmptyWorld")), WorldName(InWorldName)
{
}

FOUUAutomationTestWorld::~FOUUAutomationTestWorld()
{
	if (!ensureMsgf(
			!bHasWorld,
			TEXT("Undestroyed world found! You must always explicitly delete/cleanup automation test worlds that are "
				 "not FScopedAutomationTestWorlds!!!")))
	{
		// Explicitly call FOUUAutomationTestWorld version of the virtual function
		// -> happens anyways because it's called in destructor
		FOUUAutomationTestWorld::DestroyWorld();
	}
}

void FOUUAutomationTestWorld::CreateWorld(const FString& WorldSuffix)
{
	CreateWorldImplementation(WorldSuffix);
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

	#define CHECK_INIT_GAME_CONDITION(Condition, ErrorString)                                                          \
		if (Condition)                                                                                                 \
		{                                                                                                              \
			UE_LOG(LogOpenUnrealUtilities, Error, TEXT("%s"), ToCStr(ErrorString));                                    \
			return false;                                                                                              \
		}

bool FOUUAutomationTestWorld::InitializeGame()
{
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Could not InitiailizeGame invalid world!"));
		return false;
	}

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

	#if UE_VERSION_OLDER_THAN(5, 0, 0)
	TSharedPtr<const FUniqueNetId> UniqueNetId = GameInstance->GetPrimaryPlayerUniqueId();
	FUniqueNetIdRepl NetIdRepl = UniqueNetId;
	#else
	FUniqueNetIdRepl NetIdRepl = GameInstance->GetPrimaryPlayerUniqueIdRepl();
	#endif

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

void FOUUAutomationTestWorld::CreateWorldImplementation(const FString& WorldSuffix)
{
	if (!ensureMsgf(
			!bHasWorld,
			TEXT("Undestroyed world found! You must always explicitly delete/cleanup automation test worlds that are "
				 "not FScopedAutomationTestWorlds!!!")))
	{
		DestroyWorldImplementation();
	}

	const FString NewWorldName = "OUUAutomationTestWorld_" + WorldName + WorldSuffix;

	auto* GameMapSettings = GetMutableDefault<UGameMapsSettings>();
	PreviousDefaultMap = GameMapSettings->GetGameDefaultMap();
	GameMapSettings->SetGameDefaultMap(URL.Map);

	// Create and initialize game instance
	GameInstance = NewObject<UGameInstance>(GEngine);
	GameInstance->InitializeStandalone(*NewWorldName); // -> indiretly calls GameInstance->Init();

	World = GameInstance->GetWorld();
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
	}

	World->DestroyWorld(true);
	GEngine->DestroyWorldContext(World);

	auto* GameMapSettings = GetMutableDefault<UGameMapsSettings>();
	GameMapSettings->SetGameDefaultMap(PreviousDefaultMap);

	World = nullptr;
	GameInstance = nullptr;
	GameMode = nullptr;
	LocalPlayer = nullptr;
	PlayerController = nullptr;

	bHasWorld = false;
}

FOUUScopedAutomationTestWorld::FOUUScopedAutomationTestWorld(FString InWorldName) : FOUUAutomationTestWorld(InWorldName)
{
	CreateWorldImplementation("_SCOPED");
}

FOUUScopedAutomationTestWorld::~FOUUScopedAutomationTestWorld()
{
	DestroyWorldImplementation();
}

void FOUUScopedAutomationTestWorld::CreateWorld(const FString& WorldSuffix)
{
	ensureMsgf(
		false,
		TEXT("CreateWorld must not be called on scoped automation worlds! Let RAII take care of world "
			 "creation/destruction!"));
}

void FOUUScopedAutomationTestWorld::DestroyWorld()
{
	ensureMsgf(
		false,
		TEXT("DestroyWorld must not be called on scoped automation worlds! Let RAII take care of world "
			 "creation/destruction!"));
}

#endif
