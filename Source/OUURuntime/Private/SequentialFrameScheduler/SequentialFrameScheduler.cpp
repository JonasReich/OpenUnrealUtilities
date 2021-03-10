// Copyright (c) 2021 Jonas Reich

#include "SequentialFrameScheduler/SequentialFrameScheduler.h"

void FSequentialFrameScheduler::Tick(float DeltaTime)
{
	TickCounter++;
	DeltaTimeRingBuffer.Add(DeltaTime);
	const float PredictedDeltaTimeNextFrames = DeltaTimeRingBuffer.Average();

	RemovePendingTaskFromQueue();
	AddPendingTasksToQueue();

	if (TaskQueue.Num() <= 0)
		return;

#if WITH_GAMEPLAY_DEBUGGER
	float MaxOvertimeSeconds = 0.f;
	float MaxOvertimeFraction = 0.f;
	float SumOvertimeSeconds = 0.f;
	float SumOvertimeFraction = 0.f;
#endif
	for (auto& KeyValuePair : TaskHandlesToTaskInfos)
	{
		auto Task = KeyValuePair.Value.ToSharedRef();
		Task->Tick(DeltaTime);

#if WITH_GAMEPLAY_DEBUGGER
		const float TaskOvertimeSeconds = Task->GetOvertimeSeconds();
		const float TaskOvertimeSecondsClamped = bClampStats ? FMath::Clamp(TaskOvertimeSeconds, 0.f, MAX_FLT) : TaskOvertimeSeconds;
		SumOvertimeSeconds += TaskOvertimeSecondsClamped;
		MaxOvertimeSeconds = FMath::Max(MaxOvertimeSeconds, TaskOvertimeSecondsClamped);

		const float TaskOvertimeFraction = Task->GetOvertimeFraction();
		const float TaskOvertimeFractionClamped = bClampStats ? FMath::Clamp(TaskOvertimeFraction, 0.f, MAX_FLT) : TaskOvertimeFraction;
		SumOvertimeFraction += TaskOvertimeFractionClamped;
		MaxOvertimeFraction = FMath::Max(MaxOvertimeFraction, TaskOvertimeFractionClamped);
#endif
	}

#if WITH_GAMEPLAY_DEBUGGER
	const float NumTasksFloat = static_cast<float>(TaskHandlesToTaskInfos.Num());
	DebugData.MaxDelaySecondsRingBuffer.Add(MaxOvertimeSeconds);
	DebugData.AverageDelaySecondsRingBuffer.Add(SumOvertimeSeconds / NumTasksFloat);
	DebugData.MaxDelayFractionRingBuffer.Add(MaxOvertimeFraction);
	DebugData.AverageDelayFractionRingBuffer.Add(SumOvertimeFraction / NumTasksFloat);
#endif

	TaskQueue.Sort([&](const FTaskHandle& HandleA, const FTaskHandle& HandleB) -> bool
	{
		const FSequentialFrameTask& TaskA = GetTask(HandleA);
		const FSequentialFrameTask& TaskB = GetTask(HandleB);
		float OvertimeA = TaskA.GetOvertimeFraction();
		float OvertimeB = TaskB.GetOvertimeFraction();
		for (int32 iFrame = 1; FMath::IsNearlyEqual(OvertimeA, OvertimeB) && iFrame <= NumFramesToLookAheadForSorting; iFrame++)
		{
			OvertimeA = TaskA.GetPredictedOvertimeFraction(PredictedDeltaTimeNextFrames, iFrame);
			OvertimeB = TaskB.GetPredictedOvertimeFraction(PredictedDeltaTimeNextFrames, iFrame);
		}
		return OvertimeA > OvertimeB;
	});

	const int32 MaxNumTasksToExecuteThisFrame = FMath::Min(MaxNumTasksToExecutePerFrame, TaskQueue.Num());
	int32 ActualNumTasksExecutedThisFrame = 0; 

	for (int32 QueueIndex = 0; QueueIndex < TaskQueue.Num(); QueueIndex++)
	{
		if (ActualNumTasksExecutedThisFrame >= MaxNumTasksToExecuteThisFrame)
			break;

		TSharedRef<FSequentialFrameTask> CurrentTask = TaskHandlesToTaskInfos[TaskQueue[QueueIndex]].ToSharedRef();

		// No overtime means the task is not due yet.
		// If it's not set as "tick as often as possible" we should not pick it prematurely
		// no matter where it is in the queue.
		// This means we would have to check here anyways even if we factored it in the sorting.
		// As a result, we can just ignore the bTickAsOftenAsPossible while sorting. 
		if (!CurrentTask->bTickAsOftenAsPossible && (CurrentTask->GetOvertimeSeconds() < 0))
		{
			continue;
		}

		const float LastInvocationTimeBefore = CurrentTask->LastInvocationTime;
		CurrentTask->Execute();
		const float TaskWaitTime = CurrentTask->Now - LastInvocationTimeBefore;

		ActualNumTasksExecutedThisFrame++;

#if WITH_GAMEPLAY_DEBUGGER
		DebugData.TaskHistory.Add(TTuple<uint32, FTaskHandle, float>{TickCounter, CurrentTask->Handle, TaskWaitTime});
#endif
	}
#if WITH_GAMEPLAY_DEBUGGER
	DebugData.NumTasksExecutedRingBuffer.Add(ActualNumTasksExecutedThisFrame);
#endif
}

bool FSequentialFrameScheduler::TaskExists(const FTaskHandle& Handle) const
{
	return TaskHandlesToTaskInfos.Contains(Handle);
}

void FSequentialFrameScheduler::RemoveTask(const FTaskHandle& Handle)
{
	TasksPendingForRemoval.Add(Handle);
	TasksPendingForAdd.Remove(Handle);
}

void FSequentialFrameScheduler::AddTaskDebugName(const FTaskHandle& Handle, const FName TaskName)
{
#if WITH_GAMEPLAY_DEBUGGER
	DebugData.TaskDebugNames.Add(Handle, TaskName);
#endif
}

bool FSequentialFrameScheduler::IsTaskPaused(const FTaskHandle& Handle) const
{
	if (auto* Task = TaskHandlesToTaskInfos.Find(Handle))
	{
		return Task->Get()->bIsPaused;
	}
	return false;
}

void FSequentialFrameScheduler::PauseTask(const FTaskHandle& Handle)
{
	if (TaskExists(Handle))
	{
		GetTask(Handle).bIsPaused = true;
	}
}

void FSequentialFrameScheduler::UnPauseTask(const FTaskHandle& Handle)
{
	if (TaskExists(Handle))
	{
		GetTask(Handle).bIsPaused = false;
	}
}

FSequentialFrameTask::FTaskHandle FSequentialFrameScheduler::InternalAddTask(FTaskUnifiedDelegate&& Delegate, float InPeriod, bool bTickAsOftenAsPossible)
{
	const FTaskHandle NewHandle = TaskIdCounter;
	TaskIdCounter++;
	checkf(TaskIdCounter > 0, TEXT("overflow detected"));

	TasksPendingForAdd.Add(NewHandle);
	TasksPendingForRemoval.Remove(NewHandle);

	TSharedRef<FSequentialFrameTask> Task = MakeShared<FSequentialFrameTask>(); 
	Task->Delegate = MoveTemp(Delegate);
	Task->Handle = NewHandle;
	Task->Period = InPeriod;
	Task->bTickAsOftenAsPossible = bTickAsOftenAsPossible;
	TaskHandlesToTaskInfos.Add(NewHandle, Task);

	return NewHandle;
}

void FSequentialFrameScheduler::AddPendingTasksToQueue()
{
	for (auto TaskHandle : TasksPendingForAdd)
	{
		TaskQueue.Add(TaskHandle);
		// Pretend the task needs immediate invocation when initially adding it to the queue.
		// This mainly ensures that tasks being added after minutes/hours of play don't get unproportionally large overtime
		// and tasks added as bTickAsOftenAsPossible=false at least get the initial tick as soon as possible.
		GetTask(TaskHandle).LastInvocationTime = -1.0f * GetTask(TaskHandle).Period;
	}
	TasksPendingForAdd.Empty();
}

void FSequentialFrameScheduler::RemovePendingTaskFromQueue()
{
	for (auto TaskHandle : TasksPendingForRemoval)
	{
		TaskQueue.Remove(TaskHandle);
		TaskHandlesToTaskInfos.Remove(TaskHandle);
	}
	TasksPendingForRemoval.Empty();
}

FSequentialFrameTask& FSequentialFrameScheduler::GetTask(const FTaskHandle& Handle)
{
	return *TaskHandlesToTaskInfos.FindChecked(Handle).Get();
}
