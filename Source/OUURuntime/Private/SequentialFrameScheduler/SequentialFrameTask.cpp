// Copyright (c) 2023 Jonas Reich & Contributors

#include "SequentialFrameScheduler/SequentialFrameTask.h"

#include "SequentialFrameScheduler/SequentialFrameScheduler.h"

void FSequentialFrameTask::FTaskHandle::Cancel()
{
	auto pScheduler = pWeakScheduler.Pin();
	if (pScheduler)
	{
		pScheduler->RemoveTask(*this);
	}

	Reset();
}

float FSequentialFrameTask::GetNextDesiredInvocationTimeSeconds() const
{
	return (LastInvocationTime + Period);
}

void FSequentialFrameTask::Tick(float Now)
{
	if (bIsPaused)
		return;

	CachedOvertimeSeconds = Now - GetNextDesiredInvocationTimeSeconds();
	CachedOvertimeFraction = CachedOvertimeSeconds / GetPeriodDivisor();
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
	return CachedOvertimeFraction + ((PredictedDeltaTime / GetPeriodDivisor()) * NumFrames);
}

void FSequentialFrameTask::Execute(float Now)
{
	LastInvocationTime = Now;
	Delegate.Execute();
}

float FSequentialFrameTask::GetPeriodDivisor() const
{
	return FMath::Max(Period, UE_SMALL_NUMBER);
}
