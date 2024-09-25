// Copyright (c) 2024 Jonas Reich & Contributors

#include "GameEntitlements/Debug/GameplayDebuggerCategory_GameEntitlements.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "GameEntitlements/OUUGameEntitlements.h"
	#include "GameEntitlements/OUUGameEntitlementsSettings.h"

	#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

void FGameplayDebuggerCategory_GameEntitlements::DrawData(
	APlayerController* _pOwnerPC,
	FGameplayDebuggerCanvasContext& _CanvasContext)
{
	auto& Subsystem = UOUUGameEntitlementsSubsystem::Get();
	auto& Settings = UOUUGameEntitlementSettings::Get();
	_CanvasContext.Printf(TEXT("Default Version: %s"), *Settings.DefaultVersion.ToShortDisplayString());
	#if WITH_EDITOR
	_CanvasContext.Printf(TEXT("Default Version (Editor): %s"), *Settings.DefaultEditorVersion.ToShortDisplayString());
	#endif
	_CanvasContext.Printf(TEXT("{green}Current Version: %s"), *Subsystem.GetActiveVersion().ToShortDisplayString());
	_CanvasContext.Printf(
		TEXT("Active Entitlements:\n\t%s"),
		*Subsystem.GetActiveEntitlements().ToStringSimple().Replace(TEXT(", "), TEXT("\n\t")));
}

#endif
