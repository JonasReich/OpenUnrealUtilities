// Copyright (c) 2023 Jonas Reich & Contributors

#include "SequentialFrameScheduler/Debug/GameplayDebuggerCategory_SequentialFrameScheduler.h"

#include "Engine/Canvas.h"
#include "GameFramework/PlayerController.h"
#include "GameplayAbilities/Debug/GameplayDebuggerCategory_OUUAbilities.h"
#include "Misc/CanvasGraphPlottingUtils.h"
#include "SequentialFrameScheduler/WorldBoundSFSchedulerRegistry.h"
#include "UnrealClient.h"

#if WITH_GAMEPLAY_DEBUGGER

FString GetTickingGroupName(ETickingGroup Group)
{
	switch (Group)
	{
	case TG_PrePhysics: return "PrePhysics";
	case TG_StartPhysics: return "StartPhysics";
	case TG_DuringPhysics: return "DuringPhysics";
	case TG_EndPhysics: return "EndPhysics";
	case TG_PostPhysics: return "PostPhysics";
	case TG_PostUpdateWork: return "PostUpdateWork";
	case TG_LastDemotable: return "LastDemotable";
	case TG_NewlySpawned: return "NewlySpawned";
	default: return "<invalid>";
	}
}

FGameplayDebuggerCategory_SequentialFrameScheduler::FGameplayDebuggerCategory_SequentialFrameScheduler()
{
	bShowOnlyWithDebugActor = false;

	BindKeyPress(
		TEXT("Cycle Debug Scheduler"),
		EKeys::PageDown.GetFName(),
		FGameplayDebuggerInputModifier::None,
		this,
		&FGameplayDebuggerCategory_SequentialFrameScheduler::CycleDebugScheduler);

	BindKeyPress(
		TEXT("Add/Remove Dummy Task"),
		EKeys::Home.GetFName(),
		FGameplayDebuggerInputModifier::None,
		this,
		&FGameplayDebuggerCategory_SequentialFrameScheduler::ToggleDummyTask);
}

void FGameplayDebuggerCategory_SequentialFrameScheduler::DrawData(
	APlayerController* OwnerPC,
	FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.FontRenderInfo.bEnableShadow = true;

	PrintKeyBinds(CanvasContext);

	if (!IsValid(OwnerPC))
	{
		CanvasContext.Print(TEXT("{red}No valid player controller"));
		return;
	}

	if (bToggleDummyTask)
	{
		auto DefaultScheduler = AWorldBoundSFSchedulerRegistry::GetDefaultScheduler(OwnerPC);
		if (bDummyTaskWasSpawned)
		{
			DefaultScheduler->RemoveTask(DummyTaskHandle_1);
			DefaultScheduler->RemoveTask(DummyTaskHandle_2);
			DefaultScheduler->RemoveTask(DummyTaskHandle_3);
		}
		else
		{
			auto MakeTaskDelegate = [](auto* TypedThis, int32 TaskId) {
				return FSequentialFrameTask::FTaskDelegate::CreateRaw(
					TypedThis,
					&FGameplayDebuggerCategory_SequentialFrameScheduler::ExecuteDummyTask,
					TaskId);
			};

			DummyTaskHandle_1 = DefaultScheduler->AddNamedTask("Dummy Task 1", MakeTaskDelegate(this, 1), 1.f);
			DummyTaskHandle_2 = DefaultScheduler->AddNamedTask("Dummy Task 2", MakeTaskDelegate(this, 2), 2.f);
			DummyTaskHandle_3 = DefaultScheduler->AddNamedTask("Dummy Task 3", MakeTaskDelegate(this, 3), 3.f);
		}
		bDummyTaskWasSpawned = !bDummyTaskWasSpawned;
		bToggleDummyTask = false;
	}

	AWorldBoundSFSchedulerRegistry& FrameSchedulerRegistry =
		*AWorldBoundSFSchedulerRegistry::GetWorldSingleton(OwnerPC);
	TArray<FSchedulerPtr> Schedulers;
	FrameSchedulerRegistry.SchedulersByName.GenerateValueArray(Schedulers);

	const int32 NumSchedulers = Schedulers.Num();
	if (NumSchedulers == 0)
	{
		CanvasContext.Print(TEXT("{red}No schedulers registered"));
		return;
	}

	if (ActiveDebugSchedulerTargetIndex >= NumSchedulers)
	{
		ActiveDebugSchedulerTargetIndex = 0;
	}
	FSchedulerPtr DebugScheduler = Schedulers[ActiveDebugSchedulerTargetIndex];

	CanvasContext.Print(TEXT("{white}Registered Schedulers"));
	TArray<ETickingGroup> RegisteredTickingGroups;
	FrameSchedulerRegistry.TickGroupToSchedulerPriorityList.GetKeys(RegisteredTickingGroups);
	for (ETickingGroup TickingGroup : RegisteredTickingGroups)
	{
		CanvasContext.Printf(TEXT("{green}- %s:"), *GetTickingGroupName(TickingGroup));
		TArray<FSchedulerPtr>& SchedulersInTickingGroup =
			FrameSchedulerRegistry.TickGroupToSchedulerPriorityList[TickingGroup];
		for (FSchedulerPtr Scheduler : SchedulersInTickingGroup)
		{
			const bool bIsDebugTarget = Scheduler == DebugScheduler;
			CanvasContext.Printf(
				TEXT("{%s}\t- %s (%i)"),
				*FString(bIsDebugTarget ? "yellow" : "white"),
				*Scheduler->Name.ToString(),
				Scheduler->Priority);
		}
	}

	CanvasContext.MoveToNewLine();
	CanvasContext.Printf(TEXT("{yellow}Details: %s (%i)"), *DebugScheduler->Name.ToString(), DebugScheduler->Priority);

	const float MaxDelaySeconds = DebugScheduler->DebugData.MaxDelaySecondsRingBuffer.Max();
	const float MaxDelayFraction = DebugScheduler->DebugData.MaxDelayFractionRingBuffer.Max();
	CanvasContext.Printf(TEXT("Max Delay: %.2fms \t(%f%%)"), MaxDelaySeconds * 1000.f, MaxDelayFraction);

	const float AverageDelaySeconds = DebugScheduler->DebugData.AverageDelaySecondsRingBuffer.Average();
	const float AverageDelayFraction = DebugScheduler->DebugData.AverageDelayFractionRingBuffer.Average();
	CanvasContext.Printf(TEXT("Avg Delay: %.2fms \t(%f%%)"), AverageDelaySeconds * 1000.f, AverageDelayFraction);

	const int32 MaxNumTasksExecuted = DebugScheduler->DebugData.NumTasksExecutedRingBuffer.Max();
	CanvasContext.Printf(TEXT("Max num tasks / frame: %i"), MaxNumTasksExecuted);

	CanvasContext.MoveToNewLine();

	auto& TaskHistory = DebugScheduler->DebugData.TaskHistory;
	CanvasContext.Printf(TEXT("Task history (%i frames):"), TaskHistory.Num());
	FString HistoryString = "";
	for (int32 i = 0; i < TaskHistory.Num() && i < 20; i++)
	{
		int32 FrameNumber;
		FSequentialFrameTask::FTaskHandle TaskHandle;
		float TimeBetweenUpdates;
		float TaskDuration;
		Tie(FrameNumber, TaskHandle, TimeBetweenUpdates, TaskDuration) = TaskHistory[i];

		FName TaskName = DebugScheduler->DebugData.TaskDebugNames[TaskHandle];
		if (TSharedPtr<FSequentialFrameTask>* TaskInfo = DebugScheduler->TaskHandlesToTaskInfos.Find(TaskHandle))
		{
			HistoryString += FString::Printf(
				TEXT("\n- #%i tick time: %.2fms, period: %.2fms %s; duration: %.4fms"),
				FrameNumber,
				TimeBetweenUpdates * 1000.f,
				TaskInfo->Get()->Period * 1000.f,
				*TaskName.ToString(),
				TaskDuration * 1000.f);
		}
		else
		{
			HistoryString += "\n- invalid history entry";
		}
	}
	CanvasContext.Print(HistoryString);

	if (UCanvas* Canvas = CanvasContext.Canvas.Get())
	{
		const float GraphBottomYPos = Canvas->Canvas->GetRenderTarget()->GetSizeXY().Y / Canvas->GetDPIScale() - 50.0f;

		// tasks to execute
		{
			TArray<OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData> FrameTimesStats;
			OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData::FValueRangeRef MaxRef{
				&DebugScheduler->DebugData.TaskHistory,
				[&](const void*, int32) -> float { return DebugScheduler->MaxNumTasksToExecutePerFrame; },
				[&](const void*) -> int32 { return DebugScheduler->DebugData.NumTasksExecutedRingBuffer.Num(); }};
			FrameTimesStats.Add({MaxRef, FLinearColor::Red, "max"});

			OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData::FValueRangeRef ActualRef{
				&DebugScheduler->DebugData.TaskHistory,
				[&](const void*, int32 Idx) -> float {
					return static_cast<float>(DebugScheduler->DebugData.NumTasksExecutedRingBuffer[Idx]);
				},
				[&](const void*) -> int32 { return DebugScheduler->DebugData.NumTasksExecutedRingBuffer.Num(); }};
			FrameTimesStats.Add({ActualRef, FLinearColor::Green, "actual"});

			OUU::Runtime::CanvasGraphPlottingUtils::DrawCanvasGraph(
				Canvas->Canvas,
				80.0f,
				GraphBottomYPos - 250.f,
				FrameTimesStats,
				"# tasks per frame",
				DebugScheduler->MaxNumTasksToExecutePerFrame + 1);
		}

		// delays
		{
			TArray<OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData> DelayStats;
			DelayStats.Add({DebugScheduler->DebugData.MaxDelaySecondsRingBuffer, FLinearColor::Red, "max"});
			DelayStats.Add({DebugScheduler->DebugData.AverageDelaySecondsRingBuffer, FLinearColor::Yellow, "avg"});

			OUU::Runtime::CanvasGraphPlottingUtils::DrawCanvasGraph(
				Canvas->Canvas,
				80.f + 300.f,
				GraphBottomYPos - 250.f,
				DelayStats,
				"delays",
				0.1f);
		}

		// task times
		{
			TArray<OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData> DelayTimeStats;
			int32 i = 0;
			for (auto& Entry : DebugScheduler->TaskHandlesToTaskInfos)
			{
				i++;
				float iAsAlpha =
					static_cast<float>(i) / static_cast<float>(DebugScheduler->TaskHandlesToTaskInfos.Num());

				auto& TaskHandle = Entry.Key;
				FString TaskName = DebugScheduler->DebugData.TaskDebugNames[TaskHandle].ToString();

				TArray<OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData> FrameTimesStats;
				using TaskHistoryType = FSequentialFrameScheduler::FDebugData::TaskHistoryType;

				OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData::FValueRangeRef TaskDelayRef{
					&DebugScheduler->DebugData.TaskHistory,
					[&](const void* ContainerPtr, int32 Idx) -> float {
						auto HistoryEntry = static_cast<const TaskHistoryType*>(ContainerPtr)->operator[](Idx);
						auto EntryTaskHandle = HistoryEntry.Get<FSequentialFrameTask::FTaskHandle>();
						static float LastCachedValue = -1.f;
						if (Idx == 0)
						{
							LastCachedValue = -1.f;
						}
						if (EntryTaskHandle == TaskHandle)
						{
							LastCachedValue = HistoryEntry.Get<2>();
						}
						return LastCachedValue;
					},
					[](const void* ContainerPtr) -> int32 {
						return static_cast<const TaskHistoryType*>(ContainerPtr)->Num();
					}};

				auto UniqueDelayColor = FMath::Lerp(FLinearColor::Green, FLinearColor::Red, iAsAlpha);
				DelayTimeStats.Add({TaskDelayRef, UniqueDelayColor, TaskName});

				OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData::FValueRangeRef TaskTimeRef{
					&DebugScheduler->DebugData.TaskHistory,
					[&](const void* ContainerPtr, int32 Idx) -> float {
						auto HistoryEntry = static_cast<const TaskHistoryType*>(ContainerPtr)->operator[](Idx);
						auto EntryTaskHandle = HistoryEntry.Get<FSequentialFrameTask::FTaskHandle>();
						static float LastCachedValue = -1.f;
						if (Idx == 0)
						{
							LastCachedValue = -1.f;
						}
						if (EntryTaskHandle == TaskHandle)
						{
							LastCachedValue = HistoryEntry.Get<3>();
						}
						return LastCachedValue;
					},
					[](const void* ContainerPtr) -> int32 {
						return static_cast<const TaskHistoryType*>(ContainerPtr)->Num();
					}};
				FrameTimesStats.Add({TaskTimeRef, FLinearColor::Green, "task time"});

				OUU::Runtime::CanvasGraphPlottingUtils::DrawCanvasGraph(
					Canvas->Canvas,
					80.0f + 300.f * i,
					GraphBottomYPos,
					FrameTimesStats,
					TaskName,
					0.1f);
			}
			OUU::Runtime::CanvasGraphPlottingUtils::DrawCanvasGraph(
				Canvas->Canvas,
				80.0f,
				GraphBottomYPos,
				DelayTimeStats,
				"delays",
				1.f);
		}
	}
}

void FGameplayDebuggerCategory_SequentialFrameScheduler::CycleDebugScheduler()
{
	ActiveDebugSchedulerTargetIndex++;
}

void FGameplayDebuggerCategory_SequentialFrameScheduler::ToggleDummyTask()
{
	bToggleDummyTask = true;
}

void FGameplayDebuggerCategory_SequentialFrameScheduler::ExecuteDummyTask(int32 TaskId)
{
	FPlatformProcess::Sleep(
		FMath::FRandRange(0.01f * static_cast<float>(TaskId - 1), 0.01f * static_cast<float>(TaskId)));
}

#endif
