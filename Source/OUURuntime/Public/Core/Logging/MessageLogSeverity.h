// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "MessageLogSeverity.generated.h"

/**
 * Blueprint exposed copy of EMessageSeverity.
 */
UENUM(BlueprintType)
enum class EMessageLogSeverity : uint8
{
	CriticalError = 0,
	Error = EMessageSeverity::Error,
	PerformanceWarning = EMessageSeverity::PerformanceWarning,
	Warning = EMessageSeverity::Warning,
	Info = EMessageSeverity::Info
};

static_assert(EMessageLogSeverity::CriticalError == static_cast<EMessageLogSeverity>(EMessageSeverity::CriticalError),
	"CriticalError value must match in both enums");
