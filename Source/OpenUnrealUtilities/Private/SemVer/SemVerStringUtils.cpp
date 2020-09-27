// Copyright (c) 2020 Jonas Reich

#include "SemVer/SemVerStringUtils.h"
#include "Misc/RegexUtils.h"

bool USemVerStringLibrary::IsValidSemanticVersion(const FString& InString)
{
	return FRegexUtils::MatchesRegexExact("^(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\."
		"(0|[1-9]\\d*)(?:-((?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*)"
		"(?:\\.(?:0|[1-9]\\d*|\\d*[a-zA-Z-][0-9a-zA-Z-]*))*))?(?:\\+([0-9a-zA-Z-]+"
		"(?:\\.[0-9a-zA-Z-]+)*))?$", InString);
}
