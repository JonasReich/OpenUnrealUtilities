// Copyright (c) 2020 Jonas Reich

#include "SemVer/SemanticVersion.h"
#include "Misc/RegexUtils.h"
#include "OpenUnrealUtilities.h"

FSemanticVersion::FSemanticVersion(int32 Major, int32 Minor, int32 Patch, FSemVerPreReleaseIdentifier PreRelease /*= {}*/, FSemVerBuildMetadata Metadata /*= {}*/) :
	MajorVersion(Major), MinorVersion(Minor), PatchVersion(Patch), PreReleaseIdentifier(PreRelease), BuildMetadata(Metadata)
{
	if (MajorVersion < 0 || MinorVersion < 0 || PatchVersion < 0)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("No negative numbers allowed in SemanticVersion (%i.%i.%i). Defaulting to 0.1.0"),
			MajorVersion, MinorVersion, PatchVersion);
		(*this) = {};
	}
	else if (MajorVersion == 0 && MinorVersion == 0 && PatchVersion == 0)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("SemanticVersion components (minor, major, patch) must not all be zero. Defaulting to 0.1.0"));
		(*this) = {};
	}
}

FSemanticVersion::FSemanticVersion(const FString& SourceString, ESemVerParsingStrictness Strictness /*= ESemVerParsingStrictness::Strict*/)
{
	TryParseString(SourceString, Strictness);
}

FString FSemanticVersion::ToString() const
{
	FString Result = FString::Printf(TEXT("%i.%i.%i"), MajorVersion, MinorVersion, PatchVersion);
	FString PreReleaseIdentifierString = PreReleaseIdentifier.ToString();
	if (PreReleaseIdentifierString.Len() > 0)
	{
		Result.Append("-").Append(PreReleaseIdentifierString);
	}
	FString BuildMetadataString = BuildMetadata.ToString();
	if (BuildMetadataString.Len() > 0)
	{
		Result.Append("+").Append(BuildMetadataString);
	}
	return Result;
}

bool FSemanticVersion::TryParseString(const FString& SourceString, ESemVerParsingStrictness Strictness)
{
	if (!TryParseString_Internal(SourceString, Strictness))
	{
		(*this) = FSemanticVersion();
		return false;
	}
	return true;
}

int32 FSemanticVersion::GetMajorVersion() const
{
	return MajorVersion;
}

int32 FSemanticVersion::GetMinorVersion() const
{
	return MinorVersion;
}

int32 FSemanticVersion::GetPatchVersion() const
{
	return PatchVersion;
}

void FSemanticVersion::IncrementMajorVersion()
{
	MajorVersion++;
	MinorVersion = 0;
	PatchVersion = 0;
	PreReleaseIdentifier = {};
	BuildMetadata = {};
}

void FSemanticVersion::IncrementMinorVersion()
{
	MinorVersion++;
	PatchVersion = 0;
	PreReleaseIdentifier = {};
	BuildMetadata = {};
}

void FSemanticVersion::IncrementPatchVersion()
{
	PatchVersion++;
	PreReleaseIdentifier = {};
	BuildMetadata = {};
}

bool FSemanticVersion::TryIncrementPrereleaseVersion()
{
	BuildMetadata = {};
	return PreReleaseIdentifier.TryIncrement();
}

bool FSemanticVersion::EqualsPrecedence(const FSemanticVersion& Other) const
{
	return MajorVersion == Other.MajorVersion
		&& MinorVersion == Other.MinorVersion
		&& PatchVersion == Other.PatchVersion
		&& PreReleaseIdentifier == Other.PreReleaseIdentifier;
	// exclude build metadata from precedence check
}

bool FSemanticVersion::operator==(const FSemanticVersion& Other) const
{
	return EqualsPrecedence(Other) && BuildMetadata == Other.BuildMetadata;
}

bool FSemanticVersion::operator!=(const FSemanticVersion& Other) const
{
	return !((*this) == Other);
}

bool FSemanticVersion::operator<(const FSemanticVersion& Other) const
{
	if (MajorVersion < Other.MajorVersion)
		return true;
	if (MinorVersion < Other.MinorVersion)
		return true;
	if (PatchVersion < Other.PatchVersion)
		return true;
	if (PreReleaseIdentifier < Other.PreReleaseIdentifier)
		return true;
	else
		return false;
}

bool FSemanticVersion::operator<=(const FSemanticVersion& Other) const
{
	return !((*this) > Other);
}

bool FSemanticVersion::operator>(const FSemanticVersion& Other) const
{
	if (MajorVersion > Other.MajorVersion)
		return true;
	if (MinorVersion > Other.MinorVersion)
		return true;
	if (PatchVersion > Other.PatchVersion)
		return true;
	if (PreReleaseIdentifier > Other.PreReleaseIdentifier)
		return true;
	else
		return false;
}

bool FSemanticVersion::operator>=(const FSemanticVersion& Other) const
{
	return !((*this) < Other);
}

bool FSemanticVersion::TryParseString_Internal(const FString& SourceString, ESemVerParsingStrictness Strictness)
{
	switch (Strictness)
	{
	case ESemVerParsingStrictness::Strict:
	{
		FString VersionNumbersString, PreReleaseAndBuildNr;
		SourceString.Split("-", &VersionNumbersString, &PreReleaseAndBuildNr);
		if (!FRegexUtils::MatchesRegexExact("(0|[1-9]\\d*)\\.(0|[1-9]\\d*)\\.(0|[1-9]\\d*)", VersionNumbersString))
			return false;

		TArray<FString> VersionNumbers;
		VersionNumbersString.ParseIntoArray(VersionNumbers, TEXT("."));
		LexFromString(MajorVersion, *VersionNumbers[0]);
		LexFromString(MinorVersion, *VersionNumbers[1]);
		LexFromString(PatchVersion, *VersionNumbers[2]);

		FString PreReleaseString, BuildMetadataString;
		PreReleaseAndBuildNr.Split("+", &PreReleaseString, &BuildMetadataString);

		if (!PreReleaseIdentifier.TryParseString(PreReleaseString, Strictness))
			return false;

		if (!BuildMetadata.TryParseString(BuildMetadataString, Strictness))
			return false;

		return true;
	}
	case ESemVerParsingStrictness::Regular:
	{
		int32 FirstDigitIdx = INDEX_NONE;
		for (int32 i = 0; i < SourceString.Len(); i++)
		{
			const TCHAR& Char = SourceString.GetCharArray()[i];
			if (FChar::IsDigit(Char))
			{
				FirstDigitIdx = i;
				break;
			}
		}

		if (FirstDigitIdx == INDEX_NONE)
			return false;

		FString SourceStringWithoutPrefix = SourceString.Mid(FirstDigitIdx);

		FString VersionNumbersString, PreReleaseAndBuildNr;
		SourceStringWithoutPrefix.Split("-", &VersionNumbersString, &PreReleaseAndBuildNr);
		if (!FRegexUtils::MatchesRegexExact("\\d*\\.\\d*\\.\\d*", VersionNumbersString))
			return false;

		TArray<FString> VersionNumbers;
		VersionNumbersString.ParseIntoArray(VersionNumbers, TEXT("."));
		LexFromString(MajorVersion, *VersionNumbers[0]);
		LexFromString(MinorVersion, *VersionNumbers[1]);
		LexFromString(PatchVersion, *VersionNumbers[2]);

		FString PreReleaseString, BuildMetadataString;
		PreReleaseAndBuildNr.Split("+", &PreReleaseString, &BuildMetadataString);

		if (!PreReleaseIdentifier.TryParseString(PreReleaseString, Strictness))
			return false;

		if (!BuildMetadata.TryParseString(BuildMetadataString, Strictness))
			return false;

		return true;
	}
	default:
	{
		int32 FirstDigitIdx = INDEX_NONE;
		for (int32 i = 0; i < SourceString.Len(); i++)
		{
			const TCHAR& Char = SourceString.GetCharArray()[i];
			if (FChar::IsDigit(Char))
			{
				FirstDigitIdx = i;
				break;
			}
		}

		if (FirstDigitIdx == INDEX_NONE)
			return false;

		FString SourceStringWithoutPrefix = SourceString.Mid(FirstDigitIdx);

		FString VersionNumbersString, PreReleaseAndBuildNr;
		SourceStringWithoutPrefix.Split("-", &VersionNumbersString, &PreReleaseAndBuildNr);
		
		TArray<FString> VersionNumbers;
		VersionNumbersString.ParseIntoArray(VersionNumbers, TEXT("."));
		int32 NumVersionNumbers = VersionNumbers.Num();
		if (NumVersionNumbers > 0 && VersionNumbers[0].IsNumeric())
		{
			LexFromString(MajorVersion, *VersionNumbers[0]);
			if (NumVersionNumbers > 1 && VersionNumbers[1].IsNumeric())
			{
				LexFromString(MinorVersion, *VersionNumbers[1]);

				if (NumVersionNumbers > 2 && VersionNumbers[2].IsNumeric())
				{
					LexFromString(PatchVersion, *VersionNumbers[2]);

					// semver might have more than 3 version number components (e.g. 1.0.0.0) but only the first three are parsed
					// all other numbers will be dropped silently
				}
			}
		}
		else
		{
			return false;
		}
		
		// Find the first char that cannot be part of a pre-release and must therefore be part of the build metadata
		int32 FirstNonPreReleaseCharIdx = INDEX_NONE;
		// Also find the first whitespace char, so we can end at that position
		int32 FirstWhitespaceCharIdx = MAX_int32;

		for (int32 i = 0; i < SourceString.Len(); i++)
		{
			const TCHAR& Char = SourceString.GetCharArray()[i];
			if (FChar::IsDigit(Char) && FirstNonPreReleaseCharIdx == INDEX_NONE)
			{
				FirstNonPreReleaseCharIdx = i;
			}

			if (FChar::IsWhitespace(Char))
			{
				FirstWhitespaceCharIdx = i;
				break;
			}
		}

		FString PreReleaseString, BuildMetadataString;
		if (FirstNonPreReleaseCharIdx != INDEX_NONE)
		{
			PreReleaseString = PreReleaseAndBuildNr.Mid(0, FirstNonPreReleaseCharIdx);
			if (FirstNonPreReleaseCharIdx != MAX_int32)
			{
				BuildMetadataString = PreReleaseAndBuildNr.Mid(FirstNonPreReleaseCharIdx, FirstWhitespaceCharIdx - FirstNonPreReleaseCharIdx);
			}
		}
		else if (FirstNonPreReleaseCharIdx != MAX_int32)
		{
			PreReleaseString = PreReleaseAndBuildNr.Mid(0, FirstNonPreReleaseCharIdx);
		}
		else
		{
			PreReleaseString = PreReleaseAndBuildNr;
		}

		if (!PreReleaseIdentifier.TryParseString(PreReleaseString, Strictness))
			return false;

		if (!BuildMetadata.TryParseString(BuildMetadataString, Strictness))
			return false;

		return true;
	}
	}

	// No return path here to make sure all execution paths exit in one of the cases above
	// return false;
}
