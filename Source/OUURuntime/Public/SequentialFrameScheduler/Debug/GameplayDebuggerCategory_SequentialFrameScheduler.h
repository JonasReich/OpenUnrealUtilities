// Copyright (c) 2022 Jonas Reich

#pragma once

#if WITH_GAMEPLAY_DEBUGGER

	#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"
	#include "SequentialFrameScheduler/WorldBoundSFSchedulerRegistry.h"

class OUURUNTIME_API FGameplayDebuggerCategory_SequentialFrameScheduler : public FGameplayDebuggerCategory_OUUBase
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
	bool bToggleDummyTask = false;
	bool bDummyTaskWasSpawned = false;

	FSequentialFrameTask::FTaskHandle DummyTaskHandle_1;
	FSequentialFrameTask::FTaskHandle DummyTaskHandle_2;
	FSequentialFrameTask::FTaskHandle DummyTaskHandle_3;

	void CycleDebugScheduler();
	void ToggleDummyTask();

	void ExecuteDummyTask(int32 TaskId);
};

#endif
