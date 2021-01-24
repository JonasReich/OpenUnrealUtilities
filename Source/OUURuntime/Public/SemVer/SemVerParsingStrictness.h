// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "SemVerParsingStrictness.generated.h"

/**
 * How strict the string parsing functions should be in adhering with
 * the SemanticVersioning specification.
 * Parsing functions will always try to parse with the most restrictive logic first and continue
 * relaxing the rules until the specified parsing strictness is reached or parsing was successful.
 */
UENUM(BlueprintType)
enum class ESemVerParsingStrictness : uint8
{
	// Only allow semvar strings that 100% spec compliant.
	// Fail parsing for all suffixes, prefixes or invalid characters/digits.
	Strict,
	
	// Allow leading zeroes (e.g. "1.01.0" instead of "1.1.0").
	// Ignore any special characters except for whitespace for build metadata
	// (e.g. allow "1.0.0-alpha.30+nightly.346@94149" even though it has invalid '@' character).
	// SemVers parsed with this strictness might change in the digits (removing leading zeroes),
	// but everything else stays the same.
	Regular,
	
	// Always try to succeed.
	// Ignore invalid prefixes and suffixes to the version string.
	// Minimum requirement for success: digit.digit (e.g. "1.0b" and "Version 1.0@c" would pass).
	// SemVers parsed with this strictness are likely to change because they are turned
	// into semi-compliant semvers (digits and pre-release are compliant, build metadata will ignore special
	// characters just like after regular parsing).
	Liberal
};

FString OUURUNTIME_API LexToString(ESemVerParsingStrictness Strictness);

ENUM_RANGE_BY_VALUES(ESemVerParsingStrictness, ESemVerParsingStrictness::Strict, ESemVerParsingStrictness::Regular, ESemVerParsingStrictness::Liberal);
