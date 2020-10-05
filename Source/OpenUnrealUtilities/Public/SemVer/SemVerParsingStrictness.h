// Copyright (c) 2020 Jonas Reich

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

template<ESemVerParsingStrictness Strictness>
struct TSemVerRegex
{
	static_assert(Strictness == -1, "Strictness must be one of the three enum cases Strict, Regular or Liberal");
};

namespace SemVerRegex_Internal
{
	const FString SemVerRegex_Strict = "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)" // major.minor.patch
		"(?:-((?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\\.(?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?" // -pre_release
		"(?:\\+([0-9a-zA-Z-]+(?:\\.[0-9a-zA-Z-]+)*))?$"; // +build_metadata

	const FString SemVerRegex_Regular = "^(\\d+)(?:\\.(\\d+))(?:\\.(\\d+))" // major.minor.patch
		"(?:-((?:[0-9a-zA-Z-]*)(?:\\.(?:[0-9a-zA-Z-]*))*))?" // -pre_release
		"(?:\\+(\\S*))?$"; // +build_metadata

	const FString SemVerRegex_Liberal = "(\\d+)(?:\\.(\\d+))?(?:\\.(\\d+))?" // major.minor.patch
		"(?:-?((?:[0-9a-zA-Z-]*)(?:\\.(?:[0-9a-zA-Z-]*))*))?" // -pre_release
		"(?:\\+(\\S*))?"; // +build_metadata

	const FString NoneString = "";
}

template<>
struct TSemVerRegex<ESemVerParsingStrictness::Strict>
{
	static constexpr const FString& String() { return SemVerRegex_Internal::SemVerRegex_Strict; }
};

template<>
struct TSemVerRegex<ESemVerParsingStrictness::Regular>
{
	static constexpr const FString& String() { return SemVerRegex_Internal::SemVerRegex_Regular; }
};

template<>
struct TSemVerRegex<ESemVerParsingStrictness::Liberal>
{
	static constexpr const FString& String() { return SemVerRegex_Internal::SemVerRegex_Liberal; }
};

struct FSemVerRegex
{
	static constexpr const FString& String(ESemVerParsingStrictness Strictness)
	{
		switch (Strictness)
		{
		case ESemVerParsingStrictness::Strict:
			return TSemVerRegex<ESemVerParsingStrictness::Strict>::String();
		case ESemVerParsingStrictness::Regular:
			return TSemVerRegex<ESemVerParsingStrictness::Regular>::String();
		case ESemVerParsingStrictness::Liberal:
			return TSemVerRegex<ESemVerParsingStrictness::Liberal>::String();
		default:
			return SemVerRegex_Internal::NoneString;
		}
	}
};

FString OPENUNREALUTILITIES_API LexToString(ESemVerParsingStrictness Strictness);

ENUM_RANGE_BY_VALUES(ESemVerParsingStrictness, ESemVerParsingStrictness::Strict, ESemVerParsingStrictness::Regular, ESemVerParsingStrictness::Liberal);
