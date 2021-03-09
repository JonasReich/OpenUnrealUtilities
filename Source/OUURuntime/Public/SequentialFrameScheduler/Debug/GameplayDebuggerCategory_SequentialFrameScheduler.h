// Copyright (c) 2021 Jonas Reich

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

#include "GameplayDebuggerCategory.h"
#include "SequentialFrameScheduler/WorldBoundSFSchedulerRegistry.h"

class OUURUNTIME_API FGameplayDebuggerCategory_SequentialFrameScheduler : public FGameplayDebuggerCategory
{
public:
	FGameplayDebuggerCategory_SequentialFrameScheduler();

	using FSchedulerPtr = AWorldBoundSFSchedulerRegistry::FSchedulerPtr;

	static constexpr auto GetCategoryName() { return "SequentialFrameScheduler"; }

	// - FGameplayDebuggerCategory
	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;
	// --

private:
	int32 ActiveDebugSchedulerTargetIndex = 0;

	void CycleDebugScheduler();
};

#endif
