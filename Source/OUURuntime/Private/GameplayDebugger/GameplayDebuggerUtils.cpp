// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayDebugger/GameplayDebuggerUtils.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "GameplayDebuggerCategory.h"
	#include "GameplayDebuggerCategoryReplicator.h"
	#include "LogOpenUnrealUtilities.h"
	#include "GameplayDebuggerTypes.h"
	#include "Templates/StringUtils.h"

FString OUU::Runtime::GameplayDebuggerUtils::WrapStringToWidth(
	const FString& InString,
	const FGameplayDebuggerCanvasContext& CanvasContext,
	float TargetWidth)
{
	FString OutString;
	if (TryWrapStringToWidth(InString, OutString, CanvasContext, TargetWidth))
	{
		return OutString;
	}
	return InString;
}

bool OUU::Runtime::GameplayDebuggerUtils::TryWrapStringToWidth(
	const FString& InString,
	FString& OutString,
	const FGameplayDebuggerCanvasContext& CanvasContext,
	float TargetWidth)
{
	if (!InString.IsEmpty())
	{
		// Clamp the Width
		TargetWidth = FMath::Max(TargetWidth, 10.0f);

		float StrWidth = 0.0f, StrHeight = 0.0f;
		// Calculate the length (in pixel) of the string
		CanvasContext.MeasureString(InString, StrWidth, StrHeight);

		const int32 SubDivision = FMath::CeilToInt(StrWidth / TargetWidth);
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

FString OUU::Runtime::GameplayDebuggerUtils::CleanupName(FString Name)
{
	Name.RemoveFromStart(DEFAULT_OBJECT_PREFIX);
	Name.RemoveFromEnd(TEXT("_C"));
	return Name;
}

FString OUU::Runtime::GameplayDebuggerUtils::GetColoredBoolString(bool bBoolValue)
{
	return bBoolValue ? TEXT("{green}true") : TEXT("{red}false");
}

void OUU::Runtime::GameplayDebuggerUtils::SetCategoryEnabled(
	AGameplayDebuggerCategoryReplicator& InCategoryReplicator,
	const FGameplayDebuggerCategory& InCategoryToClose,
	bool bEnabled)
{
	const int32 NumCategories = InCategoryReplicator.GetNumCategories();
	int32 LocalCategoryId = INDEX_NONE;
	for (int32 i = 0; i < NumCategories; i++)
	{
		auto Category = InCategoryReplicator.GetCategory(i);
		if (&*Category == &InCategoryToClose)
			LocalCategoryId = i;
	}
	if (LocalCategoryId != INDEX_NONE)
	{
		InCategoryReplicator.SetCategoryEnabled(LocalCategoryId, bEnabled);
	}
	else
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("Could not set category %s enabled state (%s), because it was not found in the provided category "
				 "replicator %s"),
			*InCategoryToClose.GetCategoryName().ToString(),
			*LexToString(bEnabled),
			*LexToString(&InCategoryReplicator))
	}
}

#endif
