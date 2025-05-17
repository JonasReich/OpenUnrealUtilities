// Copyright (c) 2024 Jonas Reich & Contributors

#include "GameEntitlements/Debug/GameplayDebuggerCategory_GameEntitlements.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "GameEntitlements/OUUGameEntitlements.h"
	#include "GameEntitlements/OUUGameEntitlementsSettings.h"

	#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

void FGameplayDebuggerCategory_GameEntitlements::DrawData(
	APlayerController* OwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	auto& Subsystem = UOUUGameEntitlementsSubsystem::Get();
	auto& Settings = UOUUGameEntitlementSettings::Get();
	CanvasContext.Print(FColor::Yellow, TEXT("VERSIONS"));
	CanvasContext.Printf(TEXT("Default Version: %s"), *Settings.DefaultVersion.ToShortDisplayString());
	#if WITH_EDITOR
	CanvasContext.Printf(TEXT("Default Version (Editor): %s"), *Settings.DefaultEditorVersion.ToShortDisplayString());
	#endif
	CanvasContext.Printf(TEXT("{green}Current Version: %s"), *Subsystem.GetActiveVersion().ToShortDisplayString());

	CanvasContext.Print(TEXT(""));
	CanvasContext.Print(FColor::Yellow, TEXT("COLLECTIONS"));
	for (auto Tag : FOUUGameEntitlementModule::GetAllLeafTags())
	{
		if (Tag.MatchesTag(FOUUGameEntitlementTags::Collection::Get()))
		{
			FColor Color = Subsystem.IsEntitled(Tag) ? FColor::Green : FColor::White;
			CanvasContext.Print(Color, *Tag.ToShortDisplayString());
		}
	}

	CanvasContext.Print(TEXT(""));
	CanvasContext.Print(FColor::Yellow, TEXT("MODULES"));
	for (auto Tag : FOUUGameEntitlementModule::GetAllLeafTags())
	{
		if (Tag.MatchesTag(FOUUGameEntitlementTags::Module::Get()))
		{
			FColor Color = Subsystem.IsEntitled(Tag) ? FColor::Green : FColor::White;
			CanvasContext.Print(Color, *Tag.ToShortDisplayString());
		}
	}
}

#endif
