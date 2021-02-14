// Copyright (c) 2021 Jonas Reich

#include "SemVer/SemanticVersionBlueprintLibrary.h"

bool USemanticVersionBlueprintLibrary::TryParseSemanticVersionString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemanticVersion& OutSemanticVersion)
{
	return OutSemanticVersion.TryParseString(SourceString, Strictness);
}

void USemanticVersionBlueprintLibrary::IncrementSemanticMajorVersion(FSemanticVersion& Version)
{
	return Version.IncrementMajorVersion();
}

void USemanticVersionBlueprintLibrary::IncrementSemanticMinorVersion(FSemanticVersion& Version)
{
	return Version.IncrementMinorVersion();
}

void USemanticVersionBlueprintLibrary::IncrementSemanticPatchVersion(FSemanticVersion& Version)
{
	return Version.IncrementPatchVersion();
}

bool USemanticVersionBlueprintLibrary::TryIncrementSemanticPreReleaseVersion(FSemanticVersion& Version)
{
	return Version.TryIncrementPreReleaseVersion();
}

bool USemanticVersionBlueprintLibrary::SemanticVersionsEqualPrecedence(const FSemanticVersion& A, const FSemanticVersion& B)
{
	return A.EqualsPrecedence(B);
}

bool USemanticVersionBlueprintLibrary::Equal_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B)
{
	return A == B;
}

bool USemanticVersionBlueprintLibrary::NotEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B)
{
	return A != B;
}

bool USemanticVersionBlueprintLibrary::Less_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B)
{
	return A < B;
}

bool USemanticVersionBlueprintLibrary::LessEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B)
{
	return A <= B;
}

bool USemanticVersionBlueprintLibrary::Greater_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B)
{
	return A > B;
}

bool USemanticVersionBlueprintLibrary::GreaterEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B)
{
	return A >= B;
}

FString USemanticVersionBlueprintLibrary::Conv_SemVerString(const FSemanticVersion& InSemanticVersion)
{
	return InSemanticVersion.ToString();
}

bool USemanticVersionBlueprintLibrary::TryParseSemanticVersionReleaseIdentifierString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemVerPreReleaseIdentifier& OutReleaseIdentifier)
{
	return OutReleaseIdentifier.TryParseString(SourceString, Strictness);
}

bool USemanticVersionBlueprintLibrary::TryIncrementSemanticVersionReleaseIdentifier(FSemVerPreReleaseIdentifier& PreReleaseIdentifier)
{
	return PreReleaseIdentifier.TryIncrement();
}

bool USemanticVersionBlueprintLibrary::Equal_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B)
{
	return A == B;
}

bool USemanticVersionBlueprintLibrary::NotEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B)
{
	return A != B;
}

bool USemanticVersionBlueprintLibrary::Less_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B)
{
	return A < B;
}

bool USemanticVersionBlueprintLibrary::LessEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B)
{
	return A <= B;
}

bool USemanticVersionBlueprintLibrary::Greater_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B)
{
	return A > B;
}

bool USemanticVersionBlueprintLibrary::GreaterEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B)
{
	return A >= B;
}

FString USemanticVersionBlueprintLibrary::Conv_PreReleaseString(const FSemVerPreReleaseIdentifier& InReleaseIdentifier)
{
	return InReleaseIdentifier.ToString();
}

bool USemanticVersionBlueprintLibrary::TryParseSemVerBuildMetadataString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemVerBuildMetadata& OutBuildMetadata)
{
	return OutBuildMetadata.TryParseString(SourceString, Strictness);
}

bool USemanticVersionBlueprintLibrary::Equal_BuildMetadataBuildMetadata(const FSemVerBuildMetadata& A, const FSemVerBuildMetadata& B)
{
	return A == B;
}

bool USemanticVersionBlueprintLibrary::NotEqual_BuildMetadataBuildMetadata(const FSemVerBuildMetadata& A, const FSemVerBuildMetadata& B)
{
	return A != B;
}

FString USemanticVersionBlueprintLibrary::Conv_BuildMetadataString(const FSemVerBuildMetadata& InBuildMetadata)
{
	return InBuildMetadata.ToString();
}
