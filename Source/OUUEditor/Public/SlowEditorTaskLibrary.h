// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Misc/ScopedSlowTask.h"

#include "SlowEditorTaskLibrary.generated.h"

USTRUCT(BlueprintType)
struct FSlowEditorTaskHandle
{
	GENERATED_BODY()
public:
	FSlowEditorTaskHandle() = default;
	FSlowEditorTaskHandle(FGuid InNewGuid);
	FGuid TaskId = FGuid();
};

/**
 * Provides blueprint access to slow tasks.
 * Always uses the GWarn FFeedbackContext for all tasks.
 */
UCLASS()
class OUUEDITOR_API USlowEditorTaskLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	* Construct this scope from an amount of work to do, and a message to display
	* @param		InAmountOfWork			Arbitrary number of work units to perform (can be a percentage or number of steps).
	*										0 indicates that no progress frames are to be entered in this scope (automatically enters a frame encompassing the entire scope)
	* @param		InDefaultMessage		A message to display to the user to describe the purpose of the scope
	* @param		bInEnabled				When false, this scope will have no effect. Allows for proper scoped objects that are conditionally disabled.
	*/
	UFUNCTION(BlueprintCallable)
	static FSlowEditorTaskHandle StartSlowTask(float InAmountOfWork, FText InDefaultMessage = FText());

	/**
	* Creates a new dialog for this slow task after the given time threshold. If the task completes before this time, no dialog will be shown.
	* @param		Threshold				Time in seconds before dialog will be shown.
	* @param		bShowCancelButton		Whether to show a cancel button on the dialog or not
	* @param		bAllowInPIE				Whether to allow this dialog in PIE. If false, this dialog will not appear during PIE sessions.
	*/
	UFUNCTION(BlueprintCallable)
	static void MakeSlowTaskDialogDelayed(const FSlowEditorTaskHandle& SlowTaskHandle, float Threshold, bool bShowCancelButton = false, bool bAllowInPIE = false);

	/**
	* Creates a new dialog for this slow task, if there is currently not one open
	* @param		bShowCancelButton		Whether to show a cancel button on the dialog or not
	* @param		bAllowInPIE				Whether to allow this dialog in PIE. If false, this dialog will not appear during PIE sessions.
	*/
	UFUNCTION(BlueprintCallable)
	static void MakeSlowTaskDialog(const FSlowEditorTaskHandle& SlowTaskHandle, bool bShowCancelButton = false, bool bAllowInPIE = false);

	/**
	* Indicate that we are to enter a frame that will take up the specified amount of work. Completes any previous frames (potentially contributing to parent scopes' progress).
	* @param		ExpectedWorkThisFrame	The amount of work that will happen between now and the next frame, as a numerator of TotalAmountOfWork.
	* @param		Text					Optional text to describe this frame's purpose.
	*/
	UFUNCTION(BlueprintCallable)
	static void EnterSlowTaskProgressFrame(const FSlowEditorTaskHandle& SlowTaskHandle, float ExpectedWorkThisFrame = 1.f, FText Text = FText());

	/** End the slow editor task */
	UFUNCTION(BlueprintCallable)
	static void EndSlowTask(const FSlowEditorTaskHandle& SlowTaskHandle);

private:
	TMap<FGuid, TUniquePtr<FScopedSlowTask>> RegisteredSlowTasks;
	static USlowEditorTaskLibrary* Get();
};
