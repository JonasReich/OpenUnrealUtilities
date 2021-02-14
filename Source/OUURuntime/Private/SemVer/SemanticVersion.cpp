// Copyright (c) 2021 Jonas Reich

#include "SemVer/SemanticVersion.h"
#include "SemVer/SemVerRegex.h"
#include "Misc/RegexUtils.h"
#include "OUURuntimeModule.h"

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

bool FSemanticVersion::TryIncrementPreReleaseVersion()
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
	return ComparePrecedence_Internal(Other, true);
}

bool FSemanticVersion::operator<=(const FSemanticVersion& Other) const
{
	return !((*this) > Other);
}

bool FSemanticVersion::operator>(const FSemanticVersion& Other) const
{
	return ComparePrecedence_Internal(Other, false);
}

bool FSemanticVersion::operator>=(const FSemanticVersion& Other) const
{
	return !((*this) < Other);
}

bool FSemanticVersion::TryParseString_Internal(const FString& SourceString, ESemVerParsingStrictness Strictness)
{
	FString VersionNumbersString, PreReleaseAndBuildNr, PreReleaseString, BuildMetadataString;
	
	FRegexGroups Result;
	if (Strictness == ESemVerParsingStrictness::Liberal)
	{
		TArray<FRegexGroups> Matches = FRegexUtils::GetRegexMatchesAndGroups(FSemVerRegex::String(Strictness), 5, SourceString);
		if (Matches.Num() > 0)
		{
			Result = Matches[0];
		}
	}
	else
	{
		Result = FRegexUtils::GetRegexMatchAndGroupsExact(FSemVerRegex::String(Strictness), 5, SourceString);
	}
	
	if (!Result.IsValid())
		return false;

	if (!ensure(Result.CaptureGroups.Num() == 6))
		return false;

	LexFromString(MajorVersion, *Result.CaptureGroups[1].MatchString);
	LexFromString(MinorVersion, *Result.CaptureGroups[2].MatchString);
	LexFromString(PatchVersion, *Result.CaptureGroups[3].MatchString);

	// Check for max int
	UE_CLOG(MajorVersion == TNumericLimits<int32>::Max(), LogOpenUnrealUtilities, Warning, TEXT("MajorVersion is equal to maximum integer value after parsing. Such high version numbers are not supported!"));
	UE_CLOG(MinorVersion == TNumericLimits<int32>::Max(), LogOpenUnrealUtilities, Warning, TEXT("MinorVersion is equal to maximum integer value after parsing. Such high version numbers are not supported!"));
	UE_CLOG(PatchVersion == TNumericLimits<int32>::Max(), LogOpenUnrealUtilities, Warning, TEXT("PatchVersion is equal to maximum integer value after parsing. Such high version numbers are not supported!"));

	PreReleaseIdentifier = { Result.CaptureGroups[4].MatchString, Strictness };
	BuildMetadata = { Result.CaptureGroups[5].MatchString, Strictness };

	return true;
}

bool FSemanticVersion::ComparePrecedence_Internal(const FSemanticVersion& Other, bool bSmallerThan) const
{
	if (MajorVersion < Other.MajorVersion)
		return bSmallerThan;
	if (MajorVersion > Other.MajorVersion)
		return !bSmallerThan;
	if (MinorVersion < Other.MinorVersion)
		return bSmallerThan;
	if (MinorVersion > Other.MinorVersion)
		return !bSmallerThan;
	if (PatchVersion < Other.PatchVersion)
		return bSmallerThan;
	if (PatchVersion > Other.PatchVersion)
		return !bSmallerThan;
	if (PreReleaseIdentifier < Other.PreReleaseIdentifier)
		return bSmallerThan;
	if (PreReleaseIdentifier > Other.PreReleaseIdentifier)
		return !bSmallerThan;

	return false;
}
