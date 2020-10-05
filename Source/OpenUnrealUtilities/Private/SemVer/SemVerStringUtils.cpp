// Copyright (c) 2020 Jonas Reich

#include "SemVer/SemVerStringUtils.h"
#include "Misc/RegexUtils.h"
#include "SemVer/SemVerParsingStrictness.h"

bool USemVerStringLibrary::IsValidSemanticVersion(const FString& InString)
{
	return FRegexUtils::MatchesRegexExact(
		TSemVerRegex<ESemVerParsingStrictness::Strict>::String(),
		InString);
}
