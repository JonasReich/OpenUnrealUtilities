// Copyright (c) 2023 Jonas Reich & Contributors

#include "SemVer/SemVerStringUtils.h"

#include "Misc/RegexUtils.h"
#include "SemVer/SemVerParsingStrictness.h"
#include "SemVer/SemVerRegex.h"

bool USemVerStringLibrary::IsValidSemanticVersion(const FString& InString, ESemVerParsingStrictness ParsingStrictness)
{
	return OUU::Runtime::RegexUtils::MatchesRegexExact(FSemVerRegex::String(ParsingStrictness), InString);
}
