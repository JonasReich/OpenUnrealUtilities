// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#if WITH_GAMEPLAY_DEBUGGER
	#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

class FGameplayDebuggerCategory_GameEntitlements : public FOUUGameplayDebuggerCategory_Base
{
public:
	static auto GetCategoryName() { return TEXT("Entitlements"); }

public:
	// - FGameplayDebuggerCategory interface
	void DrawData(APlayerController* _pOwnerPC, FGameplayDebuggerCanvasContext& _CanvasContext) override;
};
#endif
