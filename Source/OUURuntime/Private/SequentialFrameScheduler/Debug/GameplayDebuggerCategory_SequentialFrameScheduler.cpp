// Copyright (c) 2021 Jonas Reich

#include "SequentialFrameScheduler/Debug/GameplayDebuggerCategory_SequentialFrameScheduler.h"
#include "SequentialFrameScheduler/WorldBoundSFSchedulerRegistry.h"

#if WITH_GAMEPLAY_DEBUGGER

FString GetTickingGroupName(ETickingGroup Group)
{
	switch(Group)
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

	const FGameplayDebuggerInputHandlerConfig KeyConfig(TEXT("Cycle Debug Scheduler"), EKeys::PageDown.GetFName());
	BindKeyPress(KeyConfig, this, &FGameplayDebuggerCategory_SequentialFrameScheduler::CycleDebugScheduler);
}

void FGameplayDebuggerCategory_SequentialFrameScheduler::DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext)
{
	CanvasContext.FontRenderInfo.bEnableShadow = true;

	if (!IsValid(OwnerPC))
	{
		CanvasContext.Print(TEXT("{red}No valid player controller"));
		return;
	}

	AWorldBoundSFSchedulerRegistry& FrameSchedulerRegistry = *AWorldBoundSFSchedulerRegistry::GetWorldSingleton(OwnerPC);
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
		TArray<FSchedulerPtr>& SchedulersInTickingGroup = FrameSchedulerRegistry.TickGroupToSchedulerPriorityList[TickingGroup];
		for (FSchedulerPtr Scheduler : SchedulersInTickingGroup)
		{
			const bool bIsDebugTarget = Scheduler == DebugScheduler;
			CanvasContext.Printf(TEXT("{%s}\t- %s (%i)"), *FString(bIsDebugTarget ? "yellow" : "white"), *Scheduler->Name.ToString(), Scheduler->Priority);
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
		Tie(FrameNumber, TaskHandle, TimeBetweenUpdates) = TaskHistory[i];

		FName TaskName = DebugScheduler->DebugData.TaskDebugNames[TaskHandle];
		TSharedPtr<FSequentialFrameTask> TaskInfo = DebugScheduler->TaskHandlesToTaskInfos[TaskHandle];
		HistoryString += FString::Printf(TEXT("\n- #%i tick time: %.2fms, period: %.2fms %s"), FrameNumber, TimeBetweenUpdates * 1000.f, TaskInfo->Period * 1000.f, *TaskName.ToString());
	}
	CanvasContext.Print(HistoryString);
}

void FGameplayDebuggerCategory_SequentialFrameScheduler::CycleDebugScheduler()
{
	ActiveDebugSchedulerTargetIndex++;
}

#endif
