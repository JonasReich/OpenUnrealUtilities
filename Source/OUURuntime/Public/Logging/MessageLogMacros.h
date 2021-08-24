
// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Logging/MessageLogBlueprintLibrary.h"

/**
 * Macro for UE_LOG-like logging to the message log.
 * This version takes string formatting arguments like UE_LOG() or FString::Printf().
 * Example Usage:
 * UE_MESSAGELOG(PIE, Error, TEXT("PlayerPawnComponent is not attached to anything!"));
 */
#define UE_MESSAGELOG(Category, Severity, ...) \
	(UMessageLogBlueprintLibrary::AddTextMessageLogMessage( \
		GetMessageLogName(EMessageLogName::Category), \
		FText::FromString(FString::Printf( __VA_ARGS__ )), \
		EMessageLogSeverity::Severity))

/**
 * Macro for UE_LOG-like logging to the message log.
 * This version takes a UObject pointer and string formatting arguments like UE_LOG() or FString::Printf().
 * Example Usage:
 * UE_MESSAGELOG_OBJ(GetOwner(), PIE, Error, TEXT("PlayerPawnComponent is not attached to anything!"));
 */
#define UE_MESSAGELOG_OBJ(Object, Category, Severity, ...) \
	(UMessageLogBlueprintLibrary::AddTokenizedMessageLogMessage( \
		GetMessageLogName(EMessageLogName::Category),\
		{ \
			FMessageLogToken::Create(Object), \
			FMessageLogToken::Create(FString::Printf( __VA_ARGS__ )), \
		}, \
		EMessageLogSeverity::Severity))


/**
 * Macro for UE_LOG-like logging to the message log.
 * This version takes a list of FMessageLogTokens instead of a format string. 
 * Example Usage:
 * UE_MESSAGELOG_OBJ(PIE, Error,
 *     FMessageLogToken::Create(TEXT("PlayerPawnComponent ")),
 *     FMessageLogToken::Create(PlayerPawnComponent),
 *     FMessageLogToken::Create(TEXT("is not attached to anything!")));
 */
#define UE_MESSAGELOG_TOKENIZED(Category, Severity, ...) \
	(UMessageLogBlueprintLibrary::AddTokenizedMessageLogMessage( \
		GetMessageLogName(EMessageLogName::Category), \
		{ __VA_ARGS__ }, \
		EMessageLogSeverity::Severity))
