// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#if WITH_GAMEPLAY_DEBUGGER
	#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

class FGameplayDebuggerCategory_GameEntitlements : public FOUUGameplayDebuggerCategory_Base
{
public:
	static auto GetCategoryName() { return TEXT("Entitlements"); }

	FGameplayDebuggerCategory_GameEntitlements();

public:
	// - FGameplayDebuggerCategory interface
	void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;
};
#endif
