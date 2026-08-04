// Copyright (c) 2023 Jonas Reich & Contributors

#include "Platform/AsyncExecProcess.h"

#include "Async/Async.h"
#include "LogOpenUnrealUtilities.h"

UAsyncExecProcess* UAsyncExecProcess::AsyncExecProcess(
	const FString& URL,
	const FString& Params,
	const FString& OptionalWorkingDirectory,
	bool bWriteOutputToLog)
{
	UAsyncExecProcess* Proxy = NewObject<UAsyncExecProcess>();
	Proxy->URL = URL;
	Proxy->Params = Params;
	Proxy->OptionalWorkingDirectory = OptionalWorkingDirectory;
	Proxy->bWriteOutputToLog = bWriteOutputToLog;
	return Proxy;
}

void UAsyncExecProcess::Activate()
{
	// Keep this action alive while the background task runs. There may be no game instance to register with when this
	// is invoked from the editor, so we prevent garbage collection explicitly and release the reference on completion.
	AddToRoot();

	// Copy the parameters into the background task so we don't touch this UObject's UPROPERTY-adjacent state off the
	// game thread.
	const FString LocalURL = URL;
	const FString LocalParams = Params;
	const FString LocalWorkingDirectory = OptionalWorkingDirectory;

	Async(EAsyncExecution::Thread, [this, LocalURL, LocalParams, LocalWorkingDirectory]() {
		const TCHAR* OptionalWorkingDirectoryPtr = LocalWorkingDirectory.Len() > 0 ? *LocalWorkingDirectory : nullptr;

		UE_LOG(
			LogOpenUnrealUtilities,
			Log,
			TEXT("AsyncExecProcess: Launching platform process \"%s\" with parameters \"%s\" (working directory: %s)"),
			*LocalURL,
			*LocalParams,
			OptionalWorkingDirectoryPtr ? OptionalWorkingDirectoryPtr : *FString(TEXT("default")));

		int32 ReturnCode = 0;
		FString StdOut;
		FString StdErr;
		const bool bSuccess = FPlatformProcess::ExecProcess(
			*LocalURL,
			*LocalParams,
			&ReturnCode,
			&StdOut,
			&StdErr,
			OptionalWorkingDirectoryPtr);

		FinishExecution(bSuccess, ReturnCode, StdOut, StdErr);
	});
}

void UAsyncExecProcess::FinishExecution(bool bSuccess, int32 ReturnCode, const FString& StdOut, const FString& StdErr)
{
	// Marshal back to the game thread before broadcasting so blueprint delegates run on the expected thread.
	if (IsInGameThread() == false)
	{
		AsyncTask(ENamedThreads::GameThread, [this, bSuccess, ReturnCode, StdOut, StdErr]() {
			FinishExecution(bSuccess, ReturnCode, StdOut, StdErr);
		});
		return;
	}

	UE_CLOG(
		!bSuccess,
		LogOpenUnrealUtilities,
		Error,
		TEXT("Failed to launch platform process \"%s\" with parameters \"%s\""),
		*URL,
		*Params);
	UE_CLOG(
		bWriteOutputToLog && StdOut.Len() > 0,
		LogOpenUnrealUtilities,
		Log,
		TEXT("AsyncExecProcess standard output:\n%s"),
		*StdOut);
	UE_CLOG(
		bWriteOutputToLog && StdErr.Len() > 0,
		LogOpenUnrealUtilities,
		Error,
		TEXT("AsyncExecProcess error output:\n%s"),
		*StdErr);
	UE_CLOG(
		bWriteOutputToLog && ReturnCode != 0,
		LogOpenUnrealUtilities,
		Warning,
		TEXT("AsyncExecProcess returned code %i"),
		ReturnCode);

	OnCompleted.Broadcast(bSuccess, ReturnCode, StdOut, StdErr);

	// The action has fulfilled its purpose; allow it to be garbage collected again.
	RemoveFromRoot();
	SetReadyToDestroy();
}
