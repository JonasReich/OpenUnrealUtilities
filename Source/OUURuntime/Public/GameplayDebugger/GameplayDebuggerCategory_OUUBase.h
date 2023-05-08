// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

// Implementation was moved to OUUGameplayDebuggerAddonBase.h
class FOUUGameplayDebuggerCategory_Base;

	#include "GameplayDebugger/OUUGameplayDebuggerAddonBase.h"

class UE_DEPRECATED(
	5.1,
	"FGameplayDebuggerCategory_OUUBase is deprecated, please use FOUUGameplayDebuggerCategory_Base instead.")
	FGameplayDebuggerCategory_OUUBase : public FOUUGameplayDebuggerCategory_Base
{
};

#endif
