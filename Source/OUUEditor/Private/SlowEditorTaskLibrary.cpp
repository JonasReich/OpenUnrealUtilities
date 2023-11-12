// Copyright (c) 2023 Jonas Reich & Contributors

#include "SlowEditorTaskLibrary.h"

#include "LogOpenUnrealUtilities.h"
#include "Misc/ScopedSlowTask.h"

FSlowEditorTaskHandle::FSlowEditorTaskHandle(FGuid InNewGuid) : TaskId(InNewGuid) {}

FSlowEditorTaskHandle USlowEditorTaskLibrary::StartSlowTask(float AmountOfWork, FText DefaultMessage)
{
	FGuid NewTaskGuid = FGuid::NewGuid();
	while (Get()->RegisteredSlowTasks.Find(NewTaskGuid) != nullptr)
	{
		NewTaskGuid = FGuid::NewGuid();
	}

	Get()->RegisteredSlowTasks.Add(NewTaskGuid, MakeUnique<FScopedSlowTask>(AmountOfWork, DefaultMessage, true));
	return FSlowEditorTaskHandle(NewTaskGuid);
}

void USlowEditorTaskLibrary::MakeSlowTaskDialogDelayed(
	const FSlowEditorTaskHandle& SlowTaskHandle,
	float Threshold,
	bool bShowCancelButton,
	bool bAllowInPIE)
{
	if (const auto* TaskPtr = Get()->RegisteredSlowTasks.Find(SlowTaskHandle.TaskId))
	{
		TaskPtr->Get()->MakeDialogDelayed(Threshold, bShowCancelButton, bAllowInPIE);
	}
	else
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("MakeSlowTaskDialogDelayed called with invalid slow task handle"));
	}
}

void USlowEditorTaskLibrary::MakeSlowTaskDialog(
	const FSlowEditorTaskHandle& SlowTaskHandle,
	bool bShowCancelButton,
	bool bAllowInPIE)
{
	if (const auto* TaskPtr = Get()->RegisteredSlowTasks.Find(SlowTaskHandle.TaskId))
	{
		TaskPtr->Get()->MakeDialogDelayed(bShowCancelButton, bAllowInPIE);
	}
	else
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("MakeSlowTaskDialog called with invalid slow task handle"));
	}
}

void USlowEditorTaskLibrary::EnterSlowTaskProgressFrame(
	const FSlowEditorTaskHandle& SlowTaskHandle,
	float ExpectedWorkThisFrame,
	FText Text)
{
	if (const auto* TaskPtr = Get()->RegisteredSlowTasks.Find(SlowTaskHandle.TaskId))
	{
		TaskPtr->Get()->EnterProgressFrame(ExpectedWorkThisFrame, Text);
	}
	else
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("EnterSlowTaskProgressFrame called with invalid slow task handle"));
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
