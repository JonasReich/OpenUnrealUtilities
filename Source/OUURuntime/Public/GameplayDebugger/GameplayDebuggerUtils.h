// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#if WITH_GAMEPLAY_DEBUGGER

class FGameplayDebuggerCanvasContext;

/**
 * Utility functions for writing custom gameplay debugger categories.
 * Mostly concerned with string handling.
 */
namespace GameplayDebuggerUtils
{
	/** Returns a copy of the the InString that is wrapped to fit the TargetWidth when displayed on the CanvasContext.
	 */
	FString OUURUNTIME_API
		WrapStringToWidth(const FString& InString, FGameplayDebuggerCanvasContext& CanvasContext, float TargetWidth);

	/**
	 * Returns a copy of the the InString that is wrapped to fit the TargetWidth when displayed on the CanvasContext.
	 * @returns false if InString is short enough that it does not need to be wrapped and OutString is empty.
	 *          This is an optimization which you can use if you want to avoid unnecessary string copies.
	 */
	bool OUURUNTIME_API TryWrapStringToWidth(
		const FString& InString,
		FString& OutString,
		FGameplayDebuggerCanvasContext& CanvasContext,
		float TargetWidth);

	/**
	 * @returns a copy of the name that has class default object prefix and class suffix removed to make names a bit
	 * more readable. Obviously, this loses some information that allows differentiating between CDOs, classes, etc. so
	 * use with care!
	 */
	FString OUURUNTIME_API CleanupName(FString Name);
} // namespace GameplayDebuggerUtils

#endif
