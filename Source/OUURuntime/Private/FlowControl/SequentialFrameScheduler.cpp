// Copyright (c) 2021 Jonas Reich

#pragma once

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
	Now += DeltaTime;
	DeltaTimeRingBuffer.Add(DeltaTime);
	const float PredictedDeltaTimeNextFrames = DeltaTimeRingBuffer.Average();

	RemovePendingTaskFromQueue();
	AddPendingTasksToQueue();

	if (TaskQueue.Num() <= 0)
		return;

	// #TODO setting
	const bool bClampStats = true;

	float MaxOvertimeSeconds = 0.f;
	float MaxOvertimeFraction = 0.f;
	float SumOvertimeSeconds = 0.f;
	float SumOvertimeFraction = 0.f;
	for (auto& KeyValuePair : TaskHandlesToTaskInfos)
	{
		auto Task = KeyValuePair.Value.ToSharedRef();
		Task->SetTimeNow(Now);

		const float TaskOvertimeSeconds = Task->GetOvertimeSeconds();
		const float TaskOvertimeSecondsClamped = bClampStats ? FMath::Clamp(TaskOvertimeSeconds, 0.f, MAX_FLT) : TaskOvertimeSeconds;
		SumOvertimeSeconds += TaskOvertimeSecondsClamped;
		MaxOvertimeSeconds = FMath::Max(MaxOvertimeSeconds, TaskOvertimeSecondsClamped);

		const float TaskOvertimeFraction = Task->GetOvertimeFraction();
		const float TaskOvertimeFractionClamped = bClampStats ? FMath::Clamp(TaskOvertimeFraction, 0.f, MAX_FLT) : TaskOvertimeFraction;
		SumOvertimeFraction += TaskOvertimeFractionClamped;
		MaxOvertimeFraction = FMath::Max(MaxOvertimeFraction, TaskOvertimeFractionClamped);
	}

	const float NumTasksFloat = static_cast<float>(TaskHandlesToTaskInfos.Num());
	MaxDelaySecondsRingBuffer.Add(MaxOvertimeSeconds);
	AverageDelaySecondsRingBuffer.Add(SumOvertimeSeconds / NumTasksFloat);
	MaxDelayFractionRingBuffer.Add(MaxOvertimeFraction);
	AverageDelayFractionRingBuffer.Add(SumOvertimeFraction / NumTasksFloat);

	TaskQueue.Sort([&](const FTaskHandle& HandleA, const FTaskHandle& HandleB) -> bool
	{
		const FSequentialFrameTask& TaskA = GetTask(HandleA);
		const FSequentialFrameTask& TaskB = GetTask(HandleB);
		float OvertimeA = TaskA.GetOvertimeFraction();
		float OvertimeB = TaskB.GetOvertimeFraction();
		// #TODO Extract to setting
		const int32 NumFramesToLookAhead = 3;
		for (int32 iFrame = 1; FMath::IsNearlyEqual(OvertimeA, OvertimeB) && iFrame <= NumFramesToLookAhead; iFrame++)
		{
			OvertimeA = TaskA.GetPredictedOvertimeFraction(PredictedDeltaTimeNextFrames, iFrame);
			OvertimeB = TaskB.GetPredictedOvertimeFraction(PredictedDeltaTimeNextFrames, iFrame);
		}
		return OvertimeA < OvertimeB;
	});

	// #TODO Extract to setting
	const int32 NumTasksToExecutePerFrame = 2;
	const int32 NumTasksToExecuteThisFrame = FMath::Min(NumTasksToExecutePerFrame, TaskQueue.Num());
	NumTasksExecutedRingBuffer.Add(NumTasksToExecuteThisFrame);
	for (int32 i = 0; i < NumTasksToExecuteThisFrame; i++)
	{
		ExecuteTask(i);
	}
}

void FSequentialFrameScheduler::RemoveTask(FTaskHandle Handle)
{
	TasksPendingForRemoval.Add(Handle);
	TasksPendingForAdd.Remove(Handle);
}

FSequentialFrameTask::FTaskHandle FSequentialFrameScheduler::InternalAddTask(FTaskUnifiedDelegate&& Delegate, float InPeriod)
{
	const FTaskHandle NewHandle = TaskIdCounter;
	TaskIdCounter++;
	checkf(TaskIdCounter > 0, TEXT("overflow detected"));

	TasksPendingForAdd.Add(NewHandle);
	TasksPendingForRemoval.Remove(NewHandle);

	TSharedRef<FSequentialFrameTask> Task = MakeShared<FSequentialFrameTask>(); 
	Task->Delegate = MoveTemp(Delegate);
	Task->Period = InPeriod;
	Task->Handle = NewHandle;
	TaskHandlesToTaskInfos.Add(NewHandle);

	return NewHandle;
}

void FSequentialFrameScheduler::AddPendingTasksToQueue()
{
	for (auto TaskHandle : TasksPendingForAdd)
	{
		TaskQueue.Add(TaskHandle);
		// Pretend the task is first invoked when initially adding it to the queue.
		// This mainly ensures that tasks being added after minutes/hours of play don't get unproportionally large overtime,
		// but should always wait max 1 period before they will be finally invoked for the first time.
		GetTask(TaskHandle).LastInvocationTime = Now;
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

void FSequentialFrameScheduler::ExecuteTask(int32 QueueIndex)
{
	TSharedRef<FSequentialFrameTask> CurrentTask = TaskHandlesToTaskInfos[TaskQueue[QueueIndex]].ToSharedRef();
	CurrentTask->LastInvocationTime = Now;
	CurrentTask->Delegate.Execute();
}
