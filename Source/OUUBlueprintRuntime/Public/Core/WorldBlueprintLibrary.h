// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Engine/World.h"
#include "WorldBlueprintLibrary.generated.h"

/** Copy of EWorldType that is Blueprint exposed */
UENUM(BlueprintType)
enum class EBlueprintWorldType : uint8
{
	/** An untyped world, in most cases this will be the vestigial worlds of streamed in sub-levels */
	None = 0,

	/** The game world */
	Game = EWorldType::Game,

	/** A world being edited in the editor */
	Editor = EWorldType::Editor,

	/** A Play In Editor world */
	PIE = EWorldType::PIE,

	/** A preview world for an editor tool */
	EditorPreview = EWorldType::EditorPreview,

	/** A preview world for a game */
	GamePreview = EWorldType::GamePreview,

	/** A minimal RPC world for a game */
	GameRPC = EWorldType::GameRPC,

	/** An editor world that was loaded but not currently being edited in the level editor */
	Inactive = EWorldType::Inactive
};

static_assert(EWorldType::None == 0, "None types must match");

/**
* Engine globals defined in CoreGlobals.h and Build.h that are not blueprint exposed via engine libraries.
* Does not expand upon existing C++ functionality but merely makes it available for blueprint use. 
*/
UCLASS()
class UWorldBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:

	/** Get the world type of a world */
	UFUNCTION(BlueprintPure)
	static EBlueprintWorldType GetWorldType(UWorld* World);
};
