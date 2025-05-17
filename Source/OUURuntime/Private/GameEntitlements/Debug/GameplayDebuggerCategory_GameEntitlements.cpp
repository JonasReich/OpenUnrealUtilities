// Copyright (c) 2024 Jonas Reich & Contributors

#include "GameEntitlements/Debug/GameplayDebuggerCategory_GameEntitlements.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "GameEntitlements/OUUGameEntitlements.h"
	#include "GameEntitlements/OUUGameEntitlementsSettings.h"

	#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

FGameplayDebuggerCategory_GameEntitlements::FGameplayDebuggerCategory_GameEntitlements()
{
	bShowOnlyWithDebugActor = false;
}

void FGameplayDebuggerCategory_GameEntitlements::DrawData(
	APlayerController* OwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	if (CanvasContext.Canvas.IsValid() == false)
	{
		return;
	}

	CanvasContext.FontRenderInfo.bEnableShadow = true;

	CanvasContext.Print(TEXT(""));
	CanvasContext.Print(TEXT(""));

	auto VersionTags = FOUUGameEntitlementVersion::GetAllLeafTags();
	auto ModuleTags = FOUUGameEntitlementModule::GetAllLeafTags();

	constexpr float BackgroundPadding = 5.0f;
	constexpr float BackgroundWidth = 400.0f;
	constexpr FLinearColor BackgroundColor(0.1f, 0.1f, 0.1f, 0.8f);

	const float BackgroundHeight = CanvasContext.GetLineHeight() * CanvasContext.Canvas->GetDPIScale()
		* (VersionTags.Num() + ModuleTags.Num() + 5);
	const FVector2D BackgroundLocation = FVector2D(BackgroundPadding, CanvasContext.CursorY - BackgroundPadding);
	const FVector2D BackgroundSize(BackgroundWidth + 2 * BackgroundPadding, BackgroundHeight + 2 * BackgroundPadding);

	FCanvasTileItem Background(FVector2D::ZeroVector, BackgroundSize, BackgroundColor);
	Background.BlendMode = SE_BLEND_Translucent;
	CanvasContext.DrawItem(Background, BackgroundLocation.X, BackgroundLocation.Y);

	auto& Subsystem = UOUUGameEntitlementsSubsystem::Get();
	auto& Settings = UOUUGameEntitlementSettings::Get();
	CanvasContext.Print(FColor::Yellow, TEXT("VERSIONS"));
	for (auto Tag : VersionTags)
	{
		const bool IsActiveVersion = Tag == Subsystem.GetActiveVersion();
		auto VersionDebugString = Tag.ToShortDisplayString();
		if (IsActiveVersion)
		{
			VersionDebugString += TEXT(" (active)");
		}
		if (Tag == Settings.DefaultVersion)
		{
			VersionDebugString += TEXT(" (game default)");
		}
	#if WITH_EDITOR
		if (Tag == Settings.DefaultEditorVersion)
		{
			VersionDebugString += TEXT(" (editor default)");
		}
	#endif
		CanvasContext.Print(IsActiveVersion ? FColor::Green : FColor::White, VersionDebugString);
	}

	CanvasContext.Print(TEXT(""));
	CanvasContext.Print(FColor::Yellow, TEXT("COLLECTIONS"));
	for (auto Tag : ModuleTags)
	{
		if (Tag.MatchesTag(FOUUGameEntitlementTags::Collection::Get()))
		{
			FColor Color = Subsystem.IsEntitled(Tag) ? FColor::Green : FColor::White;
			CanvasContext.Print(Color, *Tag.ToShortDisplayString());
		}
	}

	CanvasContext.Print(TEXT(""));
	CanvasContext.Print(FColor::Yellow, TEXT("MODULES"));
	for (auto Tag : ModuleTags)
	{
		if (Tag.MatchesTag(FOUUGameEntitlementTags::Module::Get()))
		{
			FColor Color = Subsystem.IsEntitled(Tag) ? FColor::Green : FColor::White;
			CanvasContext.Print(Color, *Tag.ToShortDisplayString());
		}
	}
}

#endif
