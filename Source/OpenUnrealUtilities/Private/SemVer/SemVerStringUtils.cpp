// Copyright (c) 2020 Jonas Reich

#include "SemVer/SemVerStringUtils.h"
#include "Misc/RegexUtils.h"
#include "SemVer/SemVerParsingStrictness.h"

bool USemVerStringLibrary::IsValidSemanticVersion(const FString& InString, ESemVerParsingStrictness ParsingStrictness)
{
	return FRegexUtils::MatchesRegexExact(FSemVerRegex::String(ParsingStrictness), InString);
}
