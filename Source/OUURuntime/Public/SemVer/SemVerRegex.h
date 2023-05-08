// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "SemVer/SemVerParsingStrictness.h"

template <ESemVerParsingStrictness Strictness>
struct TSemVerRegex
{
	static_assert(
		static_cast<int32>(Strictness) == -1,
		"Strictness must be one of the three enum cases Strict, Regular or Liberal");
};

namespace OUU::Runtime::Private::SemVerRegex
{
	const FString SemVerRegex_Strict = "^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)" // major.minor.patch
									   "(?:-((?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*)(?:\\.(?:0|[1-9]\\d*|\\d*[a-zA-"
									   "Z-][0-9a-zA-Z-]*))*))?"							// -pre_release
									   "(?:\\+([0-9a-zA-Z-]+(?:\\.[0-9a-zA-Z-]+)*))?$"; // +build_metadata

	const FString SemVerRegex_Regular = "^(\\d+)(?:\\.(\\d+))(?:\\.(\\d+))"					 // major.minor.patch
										"(?:-((?:[0-9a-zA-Z-]*)(?:\\.(?:[0-9a-zA-Z-]*))*))?" // -pre_release
										"(?:\\+(\\S*))?$";									 // +build_metadata

	const FString SemVerRegex_Liberal = "(\\d+)(?:\\.(\\d+))?(?:\\.(\\d+))?"				  // major.minor.patch
										"(?:-?((?:[0-9a-zA-Z-]*)(?:\\.(?:[0-9a-zA-Z-]*))*))?" // -pre_release
										"(?:\\+(\\S*))?";									  // +build_metadata

	const FString NoneString = "";
} // namespace OUU::Runtime::Private::SemVerRegex

template <>
struct TSemVerRegex<ESemVerParsingStrictness::Strict>
{
	static constexpr const FString& String() { return OUU::Runtime::Private::SemVerRegex::SemVerRegex_Strict; }
};

template <>
struct TSemVerRegex<ESemVerParsingStrictness::Regular>
{
	static constexpr const FString& String() { return OUU::Runtime::Private::SemVerRegex::SemVerRegex_Regular; }
};

template <>
struct TSemVerRegex<ESemVerParsingStrictness::Liberal>
{
	static constexpr const FString& String() { return OUU::Runtime::Private::SemVerRegex::SemVerRegex_Liberal; }
};

struct FSemVerRegex
{
	static constexpr const FString& String(ESemVerParsingStrictness Strictness)
	{
		switch (Strictness)
		{
		case ESemVerParsingStrictness::Strict: return TSemVerRegex<ESemVerParsingStrictness::Strict>::String();
		case ESemVerParsingStrictness::Regular: return TSemVerRegex<ESemVerParsingStrictness::Regular>::String();
		case ESemVerParsingStrictness::Liberal: return TSemVerRegex<ESemVerParsingStrictness::Liberal>::String();
		default: return OUU::Runtime::Private::SemVerRegex::NoneString;
		}
	}
};
