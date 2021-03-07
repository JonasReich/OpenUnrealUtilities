// Copyright (c) 2021 Jonas Reich

#include "FlowControl/SequentialFrameScheduler.h"

float FSequentialFrameTask::GetNextDesiredInvocationTimeSeconds() const
{
	return (LastInvocationTime + Period);
}

void FSequentialFrameTask::SetTimeNow(float Now)
{
	CachedOvertimeSeconds = Now - GetNextDesiredInvocationTimeSeconds();
	CachedOvertimeFraction = CachedOvertimeSeconds / Period;;
}

float FSequentialFrameTask::GetOvertimeSeconds() const
{
	return CachedOvertimeSeconds;
}

float FSequentialFrameTask::GetOvertimeFraction() const
{
	return CachedOvertimeFraction;
}

float FSequentialFrameTask::GetPredictedOvertimeFraction(float PredictedDeltaTime, int32 NumFrames) const
{
	return CachedOvertimeFraction + ((PredictedDeltaTime / Period) * NumFrames); 
}

void FSequentialFrameScheduler::Tick(float DeltaTime)
{
	TickCounter++;
	Now += DeltaTime;
	DeltaTimeRingBuffer.Add(DeltaTime);
	const float PredictedDeltaTimeNextFrames = DeltaTimeRingBuffer.Average();

	RemovePendingTaskFromQueue();
	AddPendingTasksToQueue();

	if (TaskQueue.Num() <= 0)
		return;

#if DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER
	float MaxOvertimeSeconds = 0.f;
	float MaxOvertimeFraction = 0.f;
	float SumOvertimeSeconds = 0.f;
	float SumOvertimeFraction = 0.f;
#endif
	for (auto& KeyValuePair : TaskHandlesToTaskInfos)
	{
		auto Task = KeyValuePair.Value.ToSharedRef();
		Task->SetTimeNow(Now);

#if DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER
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

#if DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER
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

		CurrentTask->LastInvocationTime = Now;
		CurrentTask->Delegate.Execute();

		ActualNumTasksExecutedThisFrame++;

#if DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER
		DebugData.TaskHistory.Add(TTuple<uint32, FTaskHandle>{TickCounter, CurrentTask->Handle});
#endif
	}
#if DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER
	DebugData.NumTasksExecutedRingBuffer.Add(ActualNumTasksExecutedThisFrame);
#endif
}

void FSequentialFrameScheduler::RemoveTask(const FTaskHandle& Handle)
{
	TasksPendingForRemoval.Add(Handle);
	TasksPendingForAdd.Remove(Handle);
}

void FSequentialFrameScheduler::AddTaskDebugName(const FTaskHandle& Handle, const FName TaskName)
{
#if DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER
	DebugData.TaskDebugNames.Add(Handle, TaskName);
#endif
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
		GetTask(TaskHandle).LastInvocationTime = Now - GetTask(TaskHandle).Period;
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
