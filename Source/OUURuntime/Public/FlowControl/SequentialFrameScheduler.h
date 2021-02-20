// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Templates/RingAggregator.h"

#if UE_BUILD_DEBUG || UE_BUILD_DEVELOPMENT || UE_BUILD_TEST
#define DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER 1
#else
#define DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER 0
#endif

/** A task that is registered in the SequentialFrameScheduler */
class OUURUNTIME_API FSequentialFrameTask
{
public:
	/** Handle to a task registered in the scheduler */
	struct FTaskHandle
	{
		FTaskHandle() = default;
		FTaskHandle(int32 InIndex) : Index(InIndex) {}

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

	float LastInvocationTime = 0.f;

	FTaskUnifiedDelegate Delegate;

	/** Get the next time this task wants to be invoked in seconds. */
	float GetNextDesiredInvocationTimeSeconds() const;

	void SetTimeNow(float Now);

	float GetOvertimeSeconds() const;

	/** Get the overtime for this task as a fraction of invocation period (0.5 = 50% overtime). */
	float GetOvertimeFraction() const;

	/** Get a prediction for overtime in a future number of frames */ 
	float GetPredictedOvertimeFraction(float PredictedDeltaTime, int32 NumFrames) const;

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

/**
 * The sequential frame scheduler allows organizing the invocation of time consuming tasks over multiple frames.
 *
 * Imagine a game simulation that requires system updates of various different types:
 * Each single one of these systems is required to be called regularly but not necessarily every single frame
 * (e.g. AI update every 0.2 seconds, environment queries every 0.5 seconds, etc.)
 * You can theoretically achieve this using individual timers, but especially if the individual tasks have different
 * periods, you are likely to end in an unlucky situation where two or more of those system ticks are called in the
 * same frame resulting in an ugly hitch/spike.
 *
 * This scheduler will help in these situations ensuring that only a predefined number of tasks will ever be executed in
 * the same frame (default: just a single task).
 */
class OUURUNTIME_API FSequentialFrameScheduler
{
public:
	using FTaskHandle = FSequentialFrameTask::FTaskHandle;
	using FTaskUnifiedDelegate = FSequentialFrameTask::FTaskUnifiedDelegate;
	using FTaskDelegate = FSequentialFrameTask::FTaskDelegate;
	using FTaskDynamicDelegate = FSequentialFrameTask::FTaskDynamicDelegate;

	const bool bClampStats = true;
	int32 MaxNumTasksToExecutePerFrame = 2;
	const int32 NumFramesToLookAheadForSorting = 3;

	/**
	 * Tick the frame scheduler with delta time.
	 * This function must be called a single time from one central place every frame.
	 */
	void Tick(float DeltaTime);

	/**
	 * Add a task to the scheduler.
	 * Has multiple overloads similar to the Timer Manager that allow
	 * easy binding with various different types of functions and delegates.
	 * All overloads return a task handle that can be used to remove the task again.
	 */
	template <class UserClass>
	FORCEINLINE FTaskHandle AddTask(UserClass* InObj, typename FTaskDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr InTaskMethod, float InPeriod, bool bTickAsOftenAsPossible = true)
	{
		return InternalAddTask(FTaskUnifiedDelegate(FTaskDelegate::CreateUObject(InObj, InTaskMethod)), InPeriod, bTickAsOftenAsPossible);
	}

	template <class UserClass>
	FORCEINLINE FTaskHandle AddTask(UserClass* InObj, typename FTaskDelegate::TUObjectMethodDelegate_Const<UserClass>::FMethodPtr InTaskMethod, float InPeriod, bool bTickAsOftenAsPossible = true)
	{
		return InternalAddTask(FTaskUnifiedDelegate(FTaskDelegate::CreateUObject(InObj, InTaskMethod)), InPeriod, bTickAsOftenAsPossible);
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE FTaskHandle AddTask(FTaskDelegate const& InDelegate, float InPeriod, bool bTickAsOftenAsPossible = true)
	{
		return InternalAddTask(FTaskUnifiedDelegate(InDelegate), InPeriod, bTickAsOftenAsPossible);
	}

	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE FTaskHandle AddTask(FTaskDynamicDelegate const& InDynDelegate, float InPeriod, bool bTickAsOftenAsPossible = true)
	{
		return InternalAddTask( FTaskUnifiedDelegate(InDynDelegate), InPeriod, bTickAsOftenAsPossible);
	}

	/** Version that takes a TFunction */
	FORCEINLINE FTaskHandle AddTask(TFunction<void(void)>&& Callback, float InPeriod, bool bTickAsOftenAsPossible = true)
	{
		return InternalAddTask(FTaskUnifiedDelegate(MoveTemp(Callback)), InPeriod, bTickAsOftenAsPossible);
	}

	void RemoveTask(FTaskHandle Handle);

protected:
	/**
	 * Map that point the task handles to the actual task object that store all the state of
	 * the tasks apart from it's position in the queue (see TaskQueue below).
	 */
	TMap<FTaskHandle, TSharedPtr<FSequentialFrameTask>> TaskHandlesToTaskInfos;

	/**
	 * Actively managed queue of task handles.
	 * This is the queue that is sorted and executed.
	 * The queue stores handles instead of the task objects themselves to make sorting faster.
	 */
	TArray<FTaskHandle> TaskQueue;

	/**
	 * Tasks that wait to be added to the active task queue.
	 * Used so we can add tasks at any time without disturbing task execution.
	 * Same for TasksPendingForRemoval;
	 */
	TArray<FTaskHandle> TasksPendingForAdd;
	TArray<FTaskHandle> TasksPendingForRemoval;

private:
	// Store the delta times of last 60 frames to better predict delta time for next frame
	static const int32 NumFramesBufferSize = 60;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> DeltaTimeRingBuffer;

#if DEBUG_SEQUENTIAL_FRAME_TASK_SCHEDULER
	// Various debugging metrics.
	// None of these are used by the plugin, but they will be really useful to display in a gameplay debugger
	// to show if the configuration of the debugger is balanced appropriately.
	// No gameplay debugger is provided at this time because the integration into various other systems will impact
	// how exactly the scheduler will be integrated / where it will be located.
	TFixedSizeRingAggregator<float, NumFramesBufferSize> MaxDelaySecondsRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> AverageDelaySecondsRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> MaxDelayFractionRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> AverageDelayFractionRingBuffer;
	TFixedSizeRingAggregator<int32, NumFramesBufferSize> NumTasksExecutedRingBuffer;
#endif

	// Counter to track which task IDs we already handed out.
	// Simple incrementation with overflow check should suffice as the system is
	// not really designed to be used with > INT_MAX task additions/removals.
	int32 TaskIdCounter = 0;

	// Internal time tracker. No need to manually sync this with any of the game times.
	// The scale at which this time tracker advances is directly determined by the
	// DeltaTime values passed into Tick().
	float Now = 0.f;

	FTaskHandle InternalAddTask(FTaskUnifiedDelegate&& Delegate, float InPeriod, bool bTickAsOftenAsPossible);

	void AddPendingTasksToQueue();
	void RemovePendingTaskFromQueue();

	FSequentialFrameTask& GetTask(const FTaskHandle& Handle);
};
