// Copyright (c) 2021 Jonas Reich

#include "SemVer/SemVerStringUtils.h"
#include "SemVer/SemVerParsingStrictness.h"
#include "SemVer/SemVerRegex.h"
#include "Misc/RegexUtils.h"

bool USemVerStringLibrary::IsValidSemanticVersion(const FString& InString, ESemVerParsingStrictness ParsingStrictness)
{
	return FRegexUtils::MatchesRegexExact(FSemVerRegex::String(ParsingStrictness), InString);
}
