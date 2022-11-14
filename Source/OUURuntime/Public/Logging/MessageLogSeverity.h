// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Logging/TokenizedMessage.h"

#include "MessageLogSeverity.generated.h"

/**
 * Blueprint exposed copy of EMessageSeverity.
 */
UENUM(BlueprintType)
enum class EMessageLogSeverity : uint8
{
	CriticalError UE_DEPRECATED(5.1, "CriticalError was removed.") = 0 UMETA(Hidden),
	Error = EMessageSeverity::Error,
	PerformanceWarning = EMessageSeverity::PerformanceWarning,
	Warning = EMessageSeverity::Warning,
	Info = EMessageSeverity::Info
};

static_assert(
	static_cast<uint8>(EMessageLogSeverity::Error) == 1 && static_cast<uint8>(EMessageLogSeverity::Info) == 4,
	"EMessageSeverity enum seems to have changed, please update EMessageLogSeverity to match it");
