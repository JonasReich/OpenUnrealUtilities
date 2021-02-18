// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Templates/RingAggregator.h"

struct FSequentialFrameTaskHandle
{
	int32 Idx = 0;
};

class FSequentialFrameTask
{
	FSequentialFrameTaskHandle Handle;

	float Period = 0.03f;

	// Uses unified timer delegate, because we don't want to reinvent the wheel and make it usable with everything
	// that was usable with timer manager before.
	FTimerUnifiedDelegate Delegate;

	float GetOvertimeSeconds() const;
	/** Get the overtime for this task as a percentage of invocation period */
	float GetOvertimePercent() const;

	// Movable only (required because of FTimerUnifiedDelegate)
	FSequentialFrameTask(FSequentialFrameTask&&) = default;
	FSequentialFrameTask(const FSequentialFrameTask&) = delete;
	FSequentialFrameTask& operator=(FSequentialFrameTask&&) = default;
	FSequentialFrameTask& operator=(FSequentialFrameTask&) = delete;
};

class FSequentialFrameScheduler
{
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
	/** Tick the frame scheduler with delta time */
	void Tick(float DeltaTime);
	
	template <class UserClass>
	FORCEINLINE void AddTask(FSequentialFrameTaskHandle& InOutHandle, UserClass* InObj, typename FTimerDelegate::TUObjectMethodDelegate<UserClass>::FMethodPtr InTaskMethod, float InPeriod)
	{
		InternalAddTask(InOutHandle, FTimerUnifiedDelegate(FTimerDelegate::CreateUObject(InObj, InTaskMethod)), InPeriod);
	}

	template <class UserClass>
	FORCEINLINE void AddTask(FSequentialFrameTaskHandle& InOutHandle, UserClass* InObj, typename FTimerDelegate::TUObjectMethodDelegate_Const<UserClass>::FMethodPtr InTaskMethod, float InPeriod)
	{
		InternalAddTask(InOutHandle, FTimerUnifiedDelegate(FTimerDelegate::CreateUObject(InObj, InTaskMethod)), InPeriod);
	}

	/** Version that takes any generic delegate. */
	FORCEINLINE void AddTask(FSequentialFrameTaskHandle& InOutHandle, FTimerDelegate const& InDelegate, float InPeriod)
	{
		InternalAddTask(InOutHandle, FTimerUnifiedDelegate(InDelegate), InPeriod);
	}

	/** Version that takes a dynamic delegate (e.g. for UFunctions). */
	FORCEINLINE void AddTask(FSequentialFrameTaskHandle& InOutHandle, FTimerDynamicDelegate const& InDynDelegate, float InPeriod)
	{
		InternalAddTask(InOutHandle, FTimerUnifiedDelegate(InDynDelegate), InPeriod);
	}

	/** Version that takes a TFunction */
	FORCEINLINE void AddTask(FSequentialFrameTaskHandle& InOutHandle, TFunction<void(void)>&& Callback, float InPeriod)
	{
		InternalAddTask(InOutHandle, FTimerUnifiedDelegate(MoveTemp(Callback)), InPeriod);
	}

protected:
	TMap<FSequentialFrameTaskHandle, FSequentialFrameTask> TaskHandlesToTaskInfos;
	TArray<FSequentialFrameTaskHandle> TaskQueue;

private:
	// Store the delta times of last 60 frames to better predict delta time for next frame
	static const int32 NumFramesBufferSize = 60;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> DeltaTimeRingBuffer;

	// Debugging metrics
	TFixedSizeRingAggregator<float, NumFramesBufferSize> MaxDelaySecondsRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> AverageDelaySecondsRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> MaxDelayPercentRingBuffer;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> AverageDelayPercentRingBuffer;
	TFixedSizeRingAggregator<int32, NumFramesBufferSize> NumTasksExecutedRingBuffer;

	void InternalAddTask(FSequentialFrameTaskHandle& InOutHandle, FTimerUnifiedDelegate&& Delegate, float InPeriod);
};
