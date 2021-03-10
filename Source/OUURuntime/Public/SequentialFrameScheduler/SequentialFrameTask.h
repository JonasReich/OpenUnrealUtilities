// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/** A task that is registered in the SequentialFrameScheduler */
class OUURUNTIME_API FSequentialFrameTask
{
public:
	/** Handle to a task registered in the scheduler */
	struct FTaskHandle
	{
		FTaskHandle() = default;

		FTaskHandle(int32 InIndex) : Index(InIndex)
		{
		}

		int32 Index = INDEX_NONE;

		bool operator==(const FTaskHandle& Other) const
		{
			return Index == Other.Index;
		}
	};

	/**
	 * Sequential frame tasks use extended unified timer delegates,
	 * because we don't want to reinvent the wheel and make it usable with everything
	 * that was usable with timer manager before.
	 * We can't just alias the types because then they can't be easily used in dependent types.
	 * The next best thing is inheriting from them, so here we go :)
	 */
	using FTaskUnifiedDelegate = FTimerUnifiedDelegate;
	using FTaskDelegate = FTimerDelegate;
	using FTaskDynamicDelegate = FTimerDynamicDelegate;
	//-------------------------

	FTaskHandle Handle;

	float Period = 0.03f;
	bool bTickAsOftenAsPossible = true;

	float Now = 0.f;
	float LastInvocationTime = 0.f;
	float SecondToLastInvocationTime = 0.f;

	bool bIsPaused = false;

	FTaskUnifiedDelegate Delegate;

	/** Get the next time this task wants to be invoked in seconds. */
	float GetNextDesiredInvocationTimeSeconds() const;

	void Tick(float DeltaTime);

	float GetOvertimeSeconds() const;

	/** Get the overtime for this task as a fraction of invocation period (0.5 = 50% overtime). */
	float GetOvertimeFraction() const;

	/** Get a prediction for overtime in a future number of frames */
	float GetPredictedOvertimeFraction(float PredictedDeltaTime, int32 NumFrames) const;

	void Execute();

	// Movable only (required because of FTimerUnifiedDelegate)
	FSequentialFrameTask() = default;
	FSequentialFrameTask(FSequentialFrameTask&&) = default;
	FSequentialFrameTask(const FSequentialFrameTask&) = delete;
	FSequentialFrameTask& operator=(FSequentialFrameTask&&) = default;
	FSequentialFrameTask& operator=(FSequentialFrameTask&) = delete;

private:
	float CachedOvertimeSeconds = 0.f;
	float CachedOvertimeFraction = 0.f;
};

FORCEINLINE uint32 OUURUNTIME_API GetTypeHash(const FSequentialFrameTask::FTaskHandle& Handle)
{
	return Handle.Index;
}
