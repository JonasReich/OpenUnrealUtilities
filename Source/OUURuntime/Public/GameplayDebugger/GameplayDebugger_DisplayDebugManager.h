// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER

class FGameplayDebuggerCanvasContext;

/**
 * Copy of FDisplayDebugManager that is DLL exported and therefore usable in gameplay debuggers.
 * Should be used sparingly, e.g. to convert existing ShowDebug commands into gameplay debuggers.
 */
struct OUURUNTIME_API FGameplayDebugger_DisplayDebugManager
{
public:
	explicit FGameplayDebugger_DisplayDebugManager(FGameplayDebuggerCanvasContext& InCanvasContext);

	void SetDrawColor(const FColor& NewColor);

	void SetLinearDrawColor(const FLinearColor& NewColor);

	void DrawString(const FString& InDebugString, const float& OptionalXOffset = 0.f);

	void AddColumnIfNeeded();

	float GetYStep() const;

	float GetXPos() const;

	float GetYPos() const;

	void SetYPos(const float NewYPos);

	float GetMaxCharHeight() const;

	void ShiftYDrawPosition(const float& YOffset);

private:
	float NextColumnXPos = 0.f;
	FColor DrawColor;

	float MaxCursorY = 0.f;

	FGameplayDebuggerCanvasContext& CanvasContext;
};

#endif
