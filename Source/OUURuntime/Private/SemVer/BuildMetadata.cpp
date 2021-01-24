// Copyright (c) 2021 Jonas Reich

#include "SemVer/BuildMetadata.h"
#include "Misc/RegexUtils.h"

FSemVerBuildMetadata::FSemVerBuildMetadata(const FString& SourceString, ESemVerParsingStrictness InStrictness /*= ESemVerParsingStrictness::Strict*/)
{
	TryParseString(SourceString, InStrictness);
}

FString FSemVerBuildMetadata::ToString() const
{
	return Metadata;
}

bool FSemVerBuildMetadata::TryParseString(const FString& SourceString, ESemVerParsingStrictness InStrictness)
{
	Strictness = InStrictness;
	if (SourceString.Len() == 0)
	{
		// Empty string is always ok
		Metadata = SourceString;
		return true;
	}

	if (Strictness == ESemVerParsingStrictness::Strict)
	{
		if (FRegexUtils::MatchesRegexExact("([0-9a-zA-Z-]+(?:\\.[0-9a-zA-Z-]+)*)", SourceString))
		{
			Metadata = SourceString;
			return true;
		}
	}
	else if(Strictness == ESemVerParsingStrictness::Regular)
	{
		FString StringCopy = SourceString.TrimStartAndEnd();
		int32 WhitespaceIdx = StringCopy.FindLastCharByPredicate(FChar::IsWhitespace);
		if (WhitespaceIdx == INDEX_NONE) 
		{
			Metadata = MoveTemp(StringCopy);
			return true;
		}
	}
	else
	{
		TArray<FString> WhitespaceSplit;
		SourceString.ParseIntoArrayWS(WhitespaceSplit);
		if (WhitespaceSplit.Num() > 0)
		{
			Metadata = WhitespaceSplit[0];
			return true;
		}
	}

	Strictness = ESemVerParsingStrictness::Strict;
	Metadata = "";
	return false;
}

bool FSemVerBuildMetadata::operator==(const FSemVerBuildMetadata& Other) const
{
	return Metadata == Other.Metadata;
}

bool FSemVerBuildMetadata::operator!=(const FSemVerBuildMetadata& Other) const
{
	return !((*this) == Other);
}
