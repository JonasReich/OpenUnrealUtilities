// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "SequentialFrameScheduler/SequentialFrameTask.h"
#include "Templates/RingAggregator.h"

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
#if WITH_GAMEPLAY_DEBUGGER
	friend class FGameplayDebuggerCategory_SequentialFrameScheduler;
#endif

public:
	using FTaskHandle = FSequentialFrameTask::FTaskHandle;
	using FTaskUnifiedDelegate = FSequentialFrameTask::FTaskUnifiedDelegate;
	using FTaskDelegate = FSequentialFrameTask::FTaskDelegate;
	using FTaskDynamicDelegate = FSequentialFrameTask::FTaskDynamicDelegate;

	const bool bClampStats = true;
	int32 MaxNumTasksToExecutePerFrame = 1;
	const int32 NumFramesToLookAheadForSorting = 3;

	/**
	 * Tick the frame scheduler with delta time.
	 * This function must be called a single time from one central place every frame.
	 */
	void Tick(float DeltaTime);

	bool TaskExists(const FTaskHandle& Handle) const;

	template <typename... ArgumentTypes>
	FORCEINLINE FTaskHandle AddNamedTask(const FName TaskName, ArgumentTypes ... Args)
	{
		const FTaskHandle TaskHandle = AddTask(Args...);
		AddTaskDebugName(TaskHandle, TaskName);
		return TaskHandle;
	}

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
		return InternalAddTask(FTaskUnifiedDelegate(InDynDelegate), InPeriod, bTickAsOftenAsPossible);
	}

	/** Version that takes a TFunction */
	FORCEINLINE FTaskHandle AddTask(TFunction<void(void)>&& Callback, float InPeriod, bool bTickAsOftenAsPossible = true)
	{
		return InternalAddTask(FTaskUnifiedDelegate(MoveTemp(Callback)), InPeriod, bTickAsOftenAsPossible);
	}

	void RemoveTask(const FTaskHandle& Handle);

	/** Give the task a somewhat recognizable name for debugging purposes. */
	void AddTaskDebugName(const FTaskHandle& Handle, const FName TaskName);

	bool IsTaskPaused(const FTaskHandle& Handle) const;
	void PauseTask(const FTaskHandle& Handle);
	void UnPauseTask(const FTaskHandle& Handle);

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

	// Store the delta times of last 60 frames to better predict delta time for next frame
	static const int32 NumFramesBufferSize = 60;
	TFixedSizeRingAggregator<float, NumFramesBufferSize> DeltaTimeRingBuffer;

#if WITH_GAMEPLAY_DEBUGGER
	struct FDebugData
	{
		/**
		 * Task names for identifying tasks when debugging them
		 * Assigning names is optional, so some if not all tasks may be unnamed.
		 */
		TMap<FTaskHandle, FName> TaskDebugNames;

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

		// Which tasks were actually executed in the last frames.
		// TTuple: [TickCounter, TaskId, TimeBetweenUpdates]
		TFixedSizeRingAggregator<TTuple<uint32, FTaskHandle, float>, NumFramesBufferSize> TaskHistory;
	} DebugData;
#endif

private:
	// Counter to track which task IDs we already handed out.
	// Simple incrementation with overflow check should suffice as the system is
	// not really designed to be used with > INT_MAX task additions/removals.
	int32 TaskIdCounter = 0;

	// Tick/frame counter
	uint32 TickCounter = 0;

	FTaskHandle InternalAddTask(FTaskUnifiedDelegate&& Delegate, float InPeriod, bool bTickAsOftenAsPossible);

	void AddPendingTasksToQueue();
	void RemovePendingTaskFromQueue();

	FSequentialFrameTask& GetTask(const FTaskHandle& Handle);
};
