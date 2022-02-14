// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "GenericPlatformProcessLibrary.generated.h"

/**
 * Blueprint wrapper for static utility functions from FGenericPlatformProcess
 */
UCLASS()
class OUUBLUEPRINTRUNTIME_API UGenericPlatformProcessLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Get the current working directory (only really makes sense on desktop platforms) */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Generic Platform Process")
	static FString GetCurrentWorkingDirectory();

	/**
	 * Return the path to the currently running executable
	 *
	 * @returns 	Path of the currently running executable
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Generic Platform Process")
	static FString ExecutablePath();

	/**
	 * Return the name of the currently running executable
	 *
	 * @param	bRemoveExtension	true to remove the extension of the executable name, false to leave it intact
	 * @returns 					Name of the currently running executable
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Generic Platform Process")
	static FString ExecutableName(bool bRemoveExtension = true);

	/**
	 * Executes a process, returning the return code, stdout, and stderr.
	 * This call blocks until the process has returned.
	 *
	 * @param URL						Path to the process to be launched
	 * @param Params					Parameters separated by spaces to be passed to the process
	 * @param OptionalWorkingDirectory	Use this directory path as working directory. Uses default working directory
	 *									when left empty.
	 * @param bWriteOutputToLog			If the output from the process should be written to the UE4 log
	 * @param OutReturnCode				Return code from the process
	 * @param OutStdOut					Standard output
	 * @param OutStdErr					Error output
	 *
	 * @returns							if the process was successfully created
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Generic Platform Process")
	static bool ExecProcess(
		const FString& URL,
		const FString& Params,
		const FString& OptionalWorkingDirectory,
		bool bWriteOutputToLog,
		int32& OutReturnCode,
		FString& OutStdOut,
		FString& OutStdErr);

	/**
	 * Executes a process as administrator, requesting elevation as necessary. This
	 * call blocks until the process has returned.
	 *
	 * @param URL					Path to the process to be launched
	 * @param Params				Parameters separated by spaces to be passed to the process
	 * @param OutReturnCode			Return code from the process
	 *
	 * @returns						if the process was successfully created
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Generic Platform Process")
	static bool ExecElevatedProcess(const FString& URL, const FString& Params, int32& OutReturnCode);

	/**
	 * Attempt to launch the provided file name in its default external application. Similar to
	 * FPlatformProcess::LaunchURL, with the exception that if a default application isn't found for the file, the user
	 * will be prompted with an "Open With..." dialog.
	 *
	 * @param	FileName	Name of the file to attempt to launch in its default external application
	 * @param	Params		Optional parameters to the default application
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Generic Platform Process")
	static void LaunchFileInDefaultExternalApplication_Open(const FString& FileName, const FString& Params);

	/**
	 * Attempt to launch the provided file name in its default external application. Similar to
	 * FPlatformProcess::LaunchURL, with the exception that if a default application isn't found for the file, the user
	 * will be prompted with an "Open With..." dialog.
	 *
	 * Same functionality as LaunchFileInDefaultExternalApplication_Open, but Windows allows two different programs to
	 * be associated with a file type, one for opening and one for editing the file.
	 *
	 * @param	FileName	Name of the file to attempt to launch in its default external application
	 * @param	Params		Optional parameters to the default application
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Generic Platform Process")
	static void LaunchFileInDefaultExternalApplication_Edit(const FString& FileName, const FString& Params);

	/**
	 * Attempt to "explore" the folder specified by the provided file path
	 *
	 * @param	FilePath	File path specifying a folder to explore
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Generic Platform Process")
	static void ExploreFolder(const FString& FilePath);
};
