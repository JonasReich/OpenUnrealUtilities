// Copyright (c) 2021 Jonas Reich

#include "SlowEditorTaskLibrary.h"
#include "OUUEditorModule.h"
#include "Misc/ScopedSlowTask.h"

FSlowEditorTaskHandle::FSlowEditorTaskHandle(FGuid InNewGuid):
TaskId(InNewGuid)
{
}

static TMap<FGuid, TUniquePtr<FScopedSlowTask>> RegisteredSlowTasks;

FSlowEditorTaskHandle USlowEditorTaskLibrary::StartSlowTask(float AmountOfWork, FText DefaultMessage)
{
	FGuid NewTaskGuid = FGuid::NewGuid();
	while(auto* TaskPtr = Get()->RegisteredSlowTasks.Find(NewTaskGuid))
	{
		NewTaskGuid = FGuid::NewGuid();
	}

	Get()->RegisteredSlowTasks.Add(NewTaskGuid, MakeUnique<FScopedSlowTask>(AmountOfWork, DefaultMessage, true));
	return FSlowEditorTaskHandle(NewTaskGuid);
}

void USlowEditorTaskLibrary::MakeSlowTaskDialogDelayed(const FSlowEditorTaskHandle& SlowTaskHandle, float Threshold, bool bShowCancelButton, bool bAllowInPIE)
{
	if(auto* TaskPtr = Get()->RegisteredSlowTasks.Find(SlowTaskHandle.TaskId))
	{
		TaskPtr->Get()->MakeDialogDelayed(Threshold, bShowCancelButton, bAllowInPIE);
	}
	else
	{
		UE_LOG(LogOpenUnrealUtilitiesEditor, Warning, TEXT("MakeSlowTaskDialogDelayed called with invalid slow task handle"));
	}
	
}

void USlowEditorTaskLibrary::MakeSlowTaskDialog(const FSlowEditorTaskHandle& SlowTaskHandle, bool bShowCancelButton, bool bAllowInPIE)
{
	if(auto* TaskPtr = Get()->RegisteredSlowTasks.Find(SlowTaskHandle.TaskId))
	{
		TaskPtr->Get()->MakeDialogDelayed(bShowCancelButton, bAllowInPIE);
	}
	else
	{
		UE_LOG(LogOpenUnrealUtilitiesEditor, Warning, TEXT("MakeSlowTaskDialog called with invalid slow task handle"));
	}
}

void USlowEditorTaskLibrary::EnterSlowTaskProgressFrame(const FSlowEditorTaskHandle& SlowTaskHandle, float ExpectedWorkThisFrame, FText Text)
{
	if(auto* TaskPtr = Get()->RegisteredSlowTasks.Find(SlowTaskHandle.TaskId))
	{
		TaskPtr->Get()->EnterProgressFrame(ExpectedWorkThisFrame, Text);
	}
	else
	{
		UE_LOG(LogOpenUnrealUtilitiesEditor, Warning, TEXT("EnterSlowTaskProgressFrame called with invalid slow task handle"));
	}
}

void USlowEditorTaskLibrary::EndSlowTask(const FSlowEditorTaskHandle& SlowTaskHandle)
{
	Get()->RegisteredSlowTasks.Remove(SlowTaskHandle.TaskId);
}

USlowEditorTaskLibrary* USlowEditorTaskLibrary::Get()
{
	return GetMutableDefault<USlowEditorTaskLibrary>();
}
