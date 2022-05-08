// Copyright (c) 2022 Jonas Reich

#include "LogOpenUnrealUtilitiesLibrary.h"

#include "LogOpenUnrealUtilities.h"

void ULogOpenUnrealUtilitiesLibrary::Log(const FString& Message, EOUUBlueprintLogVerbosity Verbosity)

{
#define OUU_BLUEPRINT_LOG(Verbosity)                                                                                   \
	case EOUUBlueprintLogVerbosity::Verbosity: UE_LOG(LogOpenUnrealUtilities, Verbosity, TEXT("%s"), *Message); break;

	switch (Verbosity)
	{
		OUU_BLUEPRINT_LOG(Fatal)
		OUU_BLUEPRINT_LOG(Warning)
		OUU_BLUEPRINT_LOG(Display)
		OUU_BLUEPRINT_LOG(Log)
		OUU_BLUEPRINT_LOG(Verbose)
		OUU_BLUEPRINT_LOG(VeryVerbose)
	default:;
	}

#undef OUU_BLUEPRINT_LOG
}
