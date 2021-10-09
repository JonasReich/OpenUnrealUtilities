// Copyright (c) 2021 Jonas Reich

#include "GameplayDebugger/GameplayDebuggerUtils.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerTypes.h"

FString GameplayDebuggerUtils::WrapStringToWidth(const FString& InString, FGameplayDebuggerCanvasContext& CanvasContext, float TargetWidth)
{
	FString OutString;
	if (TryWrapStringToWidth(InString, OutString, CanvasContext, TargetWidth))
	{
		return OutString;
	}
	return InString;
}

bool GameplayDebuggerUtils::TryWrapStringToWidth(const FString& InString, FString& OutString, FGameplayDebuggerCanvasContext& CanvasContext, float TargetWidth)
{
	if (!InString.IsEmpty())
	{
		// Clamp the Width
		TargetWidth = FMath::Max(TargetWidth, 10.0f);

		float StrWidth = 0.0f, StrHeight = 0.0f;
		// Calculate the length (in pixel) of the string
		CanvasContext.MeasureString(InString, StrWidth, StrHeight);

		int32 SubDivision = FMath::CeilToInt(StrWidth / TargetWidth);
		if (SubDivision > 1)
		{
			// Copy the string
			OutString = InString;
			const int32 Step = OutString.Len() / SubDivision;
			// Start sub divide if needed
			for (int32 i = SubDivision - 1; i > 0; --i)
			{
				// Insert Line Feed
				OutString.InsertAt(i * Step - 1, '\n');
			}
			return true;
		}
	}
	// No need to wrap the text 
	return false;
}

FString GameplayDebuggerUtils::CleanupName(FString Name)
{
	Name.RemoveFromStart(DEFAULT_OBJECT_PREFIX);
	Name.RemoveFromEnd(TEXT("_C"));
	return Name;
}

#endif
