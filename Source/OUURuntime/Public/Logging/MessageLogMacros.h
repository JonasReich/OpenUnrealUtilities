
// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Logging/MessageLogBlueprintLibrary.h"

/**
 * Macro for UE_LOG-like logging to the message log.
 * This version takes a list of tokenizable arguments instead of a format string.
 * Example Usage:
 * UE_MESSAGELOG(PIE, Error, TEXT("PlayerPawnComponent "), PlayerPawnComponent, TEXT("is not attached to anything!"));
 */
#define UE_MESSAGELOG(Category, Severity, ...)                                                                         \
	(UMessageLogBlueprintLibrary::AddTokenizedMessageLogMessage(                                                       \
		GetMessageLogName(EMessageLogName::Category),                                                                  \
		FMessageLogToken::CreateList(__VA_ARGS__),                                                                     \
		EMessageLogSeverity::Severity))

/**
 * Macro for UE_LOG-like logging to the message log.
 * This version takes string formatting arguments like UE_LOG() or FString::Printf().
 * Example Usage:
 * UE_MESSAGELOG_FORMAT(PIE, Error, ContextObject, TEXT("PlayerPawnComponent %s is not attached to anything!"),
 * *PlayerPawnComponent->GetName());
 */
#define UE_MESSAGELOG_FORMAT(Category, Severity, ContextObject, ...)                                                   \
	(UMessageLogBlueprintLibrary::AddTokenizedMessageLogMessage(                                                       \
		GetMessageLogName(EMessageLogName::Category),                                                                  \
		FMessageLogToken::CreateList(ContextObject, FText::FromString(FString::Printf(__VA_ARGS__))),                  \
		EMessageLogSeverity::Severity))
