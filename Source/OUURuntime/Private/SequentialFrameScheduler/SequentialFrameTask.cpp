// Copyright (c) 2021 Jonas Reich

#include "SequentialFrameScheduler/SequentialFrameTask.h"

float FSequentialFrameTask::GetNextDesiredInvocationTimeSeconds() const
{
	return (LastInvocationTime + Period);
}

void FSequentialFrameTask::Tick(float DeltaTime)
{
	if (bIsPaused)
		return;

	Now += DeltaTime;
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

void FSequentialFrameTask::Execute()
{
	LastInvocationTime = Now;
	Delegate.Execute();
}
