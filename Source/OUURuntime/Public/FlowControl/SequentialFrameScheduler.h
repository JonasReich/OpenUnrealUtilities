// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Templates/RingAggregator.h"

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
	struct FTaskUnifiedDelegate : public FTimerUnifiedDelegate
	{
		FTaskUnifiedDelegate() {};
		FTaskUnifiedDelegate(FTimerDelegate const& D) : FTimerUnifiedDelegate(D) {};
		FTaskUnifiedDelegate(FTimerDynamicDelegate const& D) : FTimerUnifiedDelegate(D) {};
		FTaskUnifiedDelegate(TFunction<void(void)>&& Callback) : FTimerUnifiedDelegate(MoveTemp(Callback)) {}
	};
	struct FTaskDelegate : public FTimerDelegate {};
	struct FTaskDynamicDelegate : public FTimerDynamicDelegate {};
	//-------------------------


	FTaskHandle Handle;

	float Period = 0.03f;
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

class OUURUNTIME_API FSequentialFrameScheduler
{
	using FTaskHandle = FSequentialFrameTask::FTaskHandle;
	using FTaskUnifiedDelegate = FSequentialFrameTask::FTaskUnifiedDelegate;
	using FTaskDelegate = FSequentialFrameTask::FTaskDelegate;
	using FTaskDynamicDelegate = FSequentialFrameTask::FTaskDynamicDelegate;
	/**
	 * Algorithm as pseudo code:
	 *
	 * prepare
	 * - enqueue all tasks in task_queue
	 *		- t.update_time = time_now
	 * - create dt_buffer[DELTA_TIME_BUFFER_SIZE] as ring buffer
	 * 
	 * for every frame:counter f:
	 * 		- push time_dt into dt_buffer
	 * 		- dt_predicted = weighted_avg(dt_buffer[]) 
	 * 		- for task t in task_queue:
	 *				- t.overtime_abs = t.update_time - time_now
	 *				- t.overtime_perc = t.overtime_abs / t.period
	 *				- t.overtime_perc_per_frame = dt_predicted / t.period
	 * 		- sort task_queue by:
	 *				1. t.overtime_perc
	 *				2. t.overtime_perc + t.overtime_perc_per_frame * 1
	 *				3. t.overtime_perc + t.overtime_perc_per_frame * n for n < MAX_FRAMES_PREDICT
	 * 		- for task t in (NUM_TASKS_PER_FRAME from top of task_queue):
	 *				- t.execute()
	 *				- t.update_time = time_now
	 *
	 * Debug data:
	 * - avg delays % and s
	 * - max delays % and s
	 * - avg number of tasks executed per frame (x frames)
	 * - min number of tasks executed in last x frames
	 * - max number of tasks executed in last x frames
	 * 
	 */
	/////////////////////////////////////////////////////////////////////////////

public:
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
	FORCEINLINE FTaskHandle AddTask(UserClass* InObj, typename FTaskDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr InTaskMethod, float InPeriod)
	{
		return InternalAddTask(FTaskUnifiedDelegate(FTaskDelegate::CreateUObject(InObj, InTaskMethod)), InPeriod);
	}

	template <class UserClass>
	FORCEINLINE FTaskHandle AddTask(UserClass* InObj, typename FTaskDelegate::TUObjectMethodDelegate_Const<UserClass>::FMethodPtr InTaskMethod, float InPeriod)
	{
		return InternalAddTask(FTaskUnifiedDelegate(FTaskDelegate::CreateUObject(InObj, InTaskMethod)), InPeriod);
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE FTaskHandle AddTask(FTaskDelegate const& InDelegate, float InPeriod)
	{
		return InternalAddTask(FTaskUnifiedDelegate(InDelegate), InPeriod);
	}

	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE FTaskHandle AddTask(FTaskDynamicDelegate const& InDynDelegate, float InPeriod)
	{
		return InternalAddTask( FTaskUnifiedDelegate(InDynDelegate), InPeriod);
	}

	/** Version that takes a TFunction */
	FORCEINLINE FTaskHandle AddTask(TFunction<void(void)>&& Callback, float InPeriod)
	{
		return InternalAddTask(FTaskUnifiedDelegate(MoveTemp(Callback)), InPeriod);
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

	// Debugging metrics
	TFixedSizeRingAggregator<float, NumFramesBufferSize> MaxDelaySecondsRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> AverageDelaySecondsRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> MaxDelayFractionRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> AverageDelayFractionRingBuffer;
	TFixedSizeRingAggregator<int32, NumFramesBufferSize> NumTasksExecutedRingBuffer;

	// Counter to track which task IDs we already handed out.
	// Simple incrementation with overflow check should suffice as the system is
	// not really designed to be used with > INT_MAX task additions/removals.
	int32 TaskIdCounter = 0;

	// Internal time tracker. No need to manually sync this with any of the game times.
	// The scale at which this time tracker advances is directly determined by the
	// DeltaTime values passed into Tick().
	float Now = 0.f;

	FTaskHandle InternalAddTask(FTaskUnifiedDelegate&& Delegate, float InPeriod);

	void AddPendingTasksToQueue();
	void RemovePendingTaskFromQueue();

	FSequentialFrameTask& GetTask(const FTaskHandle& Handle);

	void ExecuteTask(int32 QueueIndex);
};
