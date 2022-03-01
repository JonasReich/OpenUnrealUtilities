// Copyright (c) 2022 Jonas Reich

#include "Platform/GenericPlatformProcessLibrary.h"

#include "LogOpenUnrealUtilities.h"

FString UGenericPlatformProcessLibrary::GetCurrentWorkingDirectory()
{
	return FPlatformProcess::GetCurrentWorkingDirectory();
}

FString UGenericPlatformProcessLibrary::ExecutablePath()
{
	return FPlatformProcess::ExecutablePath();
}

FString UGenericPlatformProcessLibrary::ExecutableName(bool bRemoveExtension)
{
	return FPlatformProcess::ExecutableName(bRemoveExtension);
}

bool UGenericPlatformProcessLibrary::ExecProcess(
	const FString& URL,
	const FString& Params,
	const FString& OptionalWorkingDirectory,
	bool bWriteOutputToLog,
	int32& OutReturnCode,
	FString& OutStdOut,
	FString& OutStdErr)
{
	const TCHAR* OptionalWorkingDirectoryPtr = OptionalWorkingDirectory.Len() > 0 ? *OptionalWorkingDirectory : nullptr;

	UE_LOG(
		LogOpenUnrealUtilities,
		Log,
		TEXT("ExecProcess: Launching platform process \"%s\" with parameters \"%s\" (working directory: %s)"),
		*URL,
		*Params,
		OptionalWorkingDirectoryPtr ? OptionalWorkingDirectoryPtr : *FString(TEXT("default")));
	const bool bResult = FPlatformProcess::ExecProcess(
		*URL,
		*Params,
		&OutReturnCode,
		&OutStdOut,
		&OutStdErr,
		OptionalWorkingDirectoryPtr);

	UE_CLOG(
		!bResult,
		LogOpenUnrealUtilities,
		Error,
		TEXT("Failed to launch platform process \"%s\" with parameters \"%s\""),
		*URL,
		*Params);
	UE_CLOG(
		bWriteOutputToLog && OutStdOut.Len() > 0,
		LogOpenUnrealUtilities,
		Log,
		TEXT("ExecProcess standard output:\n%s"),
		*OutStdOut);
	UE_CLOG(
		bWriteOutputToLog && OutStdErr.Len() > 0,
		LogOpenUnrealUtilities,
		Error,
		TEXT("ExecProcess error output:\n%s"),
		*OutStdErr);
	UE_CLOG(
		bWriteOutputToLog && OutReturnCode != 0,
		LogOpenUnrealUtilities,
		Warning,
		TEXT("ExecProcess returned code %i"),
		OutReturnCode);

	return bResult;
}

bool UGenericPlatformProcessLibrary::ExecElevatedProcess(
	const FString& URL,
	const FString& Params,
	int32& OutReturnCode)
{
	UE_LOG(
		LogOpenUnrealUtilities,
		Log,
		TEXT("ExecProcess: Launching platform process \"%s\" with parameters \"%s\" and elevated permissions"),
		*URL,
		*Params);
	const bool bResult = FPlatformProcess::ExecElevatedProcess(*URL, *Params, &OutReturnCode);

	UE_CLOG(
		!bResult,
		LogOpenUnrealUtilities,
		Error,
		TEXT("Failed to launch platform process \"%s\" with parameters \"%s\""),
		*URL,
		*Params);

	return bResult;
}

void UGenericPlatformProcessLibrary::LaunchFileInDefaultExternalApplication_Open(
	const FString& FileName,
	const FString& Params)
{
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*FileName, *Params, ELaunchVerb::Open);
}

void UGenericPlatformProcessLibrary::LaunchFileInDefaultExternalApplication_Edit(
	const FString& FileName,
	const FString& Params)
{
	FPlatformProcess::LaunchFileInDefaultExternalApplication(*FileName, *Params, ELaunchVerb::Edit);
}

void UGenericPlatformProcessLibrary::ExploreFolder(const FString& FilePath)
{
	return FPlatformProcess::ExploreFolder(*FilePath);
}
