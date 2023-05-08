// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "LogOpenUnrealUtilitiesLibrary.generated.h"

UENUM()
enum class EOUUBlueprintLogVerbosity : uint8
{
	/** Always prints a fatal error to console (and log file) and crashes (even if logging is disabled) */
	Fatal = ELogVerbosity::Fatal,

	/**
	 * Prints an error to console (and log file).
	 * Commandlets and the editor collect and report errors. Error messages result in commandlet failure.
	 */
	Error = ELogVerbosity::Error,

	/**
	 * Prints a warning to console (and log file).
	 * Commandlets and the editor collect and report warnings. Warnings can be treated as an error.
	 */
	Warning = ELogVerbosity::Warning,

	/** Prints a message to console (and log file) */
	Display = ELogVerbosity::Display,

	/** Prints a message to a log file (does not print to console) */
	Log = ELogVerbosity::Log,

	/**
	 * Prints a verbose message to a log file (if Verbose logging is enabled for the given category,
	 * usually used for detailed logging)
	 */
	Verbose = ELogVerbosity::Verbose,

	/**
	 * Prints a verbose message to a log file (if VeryVerbose logging is enabled,
	 * usually used for detailed logging that would otherwise spam output)
	 */
	VeryVerbose = ELogVerbosity::VeryVerbose
};

/**
 * To expose OUU logging to Blueprint
 */
UCLASS()
class OUUBLUEPRINTRUNTIME_API ULogOpenUnrealUtilitiesLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(
		BlueprintCallable,
		Category = "Open Unreal Utilities|Logging",
		meta = (DisplayName = "Log (Open Unreal Utilities)"))
	static void Log(const FString& Message, EOUUBlueprintLogVerbosity Verbosity = EOUUBlueprintLogVerbosity::Log);
};
