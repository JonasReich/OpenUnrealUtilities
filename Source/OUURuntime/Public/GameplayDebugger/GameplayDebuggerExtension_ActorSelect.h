// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayDebugger/OUUGameplayDebuggerAddonBase.h"

#if WITH_GAMEPLAY_DEBUGGER

/**
 * Extension to the gameplay debugger that allows setting common actors as gameplay debugger targets on key-press.
 * Supported targets:
 * - Local player pawn
 * - Player pawns (1-4)
 * - Nearest non-player pawn to player
 */
class FGameplayDebuggerExtension_ActorSelect : public FOUUGameplayDebuggerExtension_Base
{
public:
	FGameplayDebuggerExtension_ActorSelect();

private:
	void SelectLocalPlayerPawn();

	void SelectClosestNPC();
	
	void SelectPlayerPawn_1();
	void SelectPlayerPawn_2();
	void SelectPlayerPawn_3();
	void SelectPlayerPawn_4();

	void SelectPlayerPawn_X(int32 X);
};

#endif
