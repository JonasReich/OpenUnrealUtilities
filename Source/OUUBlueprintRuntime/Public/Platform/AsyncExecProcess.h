// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintAsyncActionBase.h"

#include "AsyncExecProcess.generated.h"

/**
 * Broadcast when an asynchronous process execution finishes (or fails to launch).
 *
 * @param	bSuccess		true if the process was successfully created and run to completion
 * @param	ReturnCode		Return code from the process (0 if the process could not be created)
 * @param	StdOut			Standard output captured from the process
 * @param	StdErr			Error output captured from the process
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(
	FOnAsyncExecProcessCompleted,
	bool,
	bSuccess,
	int32,
	ReturnCode,
	const FString&,
	StdOut,
	const FString&,
	StdErr);

/**
 * Async blueprint node that executes an external process without blocking the calling (game/editor) thread.
 * The blocking process call runs on a background thread and the result is delivered back on the game thread.
 */
UCLASS()
class OUUBLUEPRINTRUNTIME_API UAsyncExecProcess : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	/** Broadcast on the game thread once the process has returned (or failed to launch). */
	UPROPERTY(BlueprintAssignable)
	FOnAsyncExecProcessCompleted OnCompleted;

	/**
	 * Executes a process asynchronously, returning the return code, stdout, and stderr once it completes.
	 * Unlike the blocking ExecProcess node, this does NOT block the calling thread while the process runs,
	 * which makes it safe to use for long running processes in the editor.
	 *
	 * @param	URL							Path to the process to be launched
	 * @param	Params						Parameters separated by spaces to be passed to the process
	 * @param	OptionalWorkingDirectory	Use this directory path as working directory. Uses default working directory
	 *										when left empty.
	 * @param	bWriteOutputToLog			If the output from the process should be written to the log
	 *
	 * @returns								The async action object driving the process execution
	 */
	UFUNCTION(
		BlueprintCallable,
		meta = (BlueprintInternalUseOnly = "true"),
		Category = "Open Unreal Utilities|Generic Platform Process")
	static UAsyncExecProcess* AsyncExecProcess(
		const FString& URL,
		const FString& Params,
		const FString& OptionalWorkingDirectory,
		bool bWriteOutputToLog);

	// - UBlueprintAsyncActionBase
	void Activate() override;
	// --
private:
	FString URL;
	FString Params;
	FString OptionalWorkingDirectory;
	bool bWriteOutputToLog = false;

	// Marshal the result back to the game thread and broadcast the completion delegate.
	void FinishExecution(bool bSuccess, int32 ReturnCode, const FString& StdOut, const FString& StdErr);
};
