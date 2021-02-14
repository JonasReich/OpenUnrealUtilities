// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "SemVer/PreReleaseIdentifier.h"
#include "SemVer/BuildMetadata.h"
#include "SemVer/SemVerParsingStrictness.h"
#include "SemanticVersion.generated.h"

//////////////////////////////////////////////////////////////////////////
// The types defined in this file and all other in this directory implement 
// Semantic Versioning 2.0.0 as specified at https://semver.org/spec/v2.0.0.html
//////////////////////////////////////////////////////////////////////////

/**
 * A semantic version for software. Usable for games, plugins, tools, etc.
 * Summary from the Semantic Versioning 2.0.0 specification:
 * ----
 * Given a version number MAJOR.MINOR.PATCH, increment the:
 *
 * 1. MAJOR version when you make incompatible API changes,
 * 2. MINOR version when you add functionality in a backwards compatible manner, and
 * 3. PATCH version when you make backwards compatible bug fixes.
 *
 * Additional labels for pre-release and build metadata are available as extensions to the MAJOR.MINOR.PATCH format.
 * ----
 * Check SemanticVersion.spec.cpp in the tests module for examples and usage examples.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FSemanticVersion
{
	GENERATED_BODY()
public:
	/**
	 * The default constructed SemVer has 0.1.0 as version because the SemVer spec
	 * recommends using it for beginning a new version history.
	 */
	FSemanticVersion() = default;

	/** Construct a SemVer from its components */
	FSemanticVersion(int32 Major, int32 Minor, int32 Patch,
		FSemVerPreReleaseIdentifier PreRelease = {},
		FSemVerBuildMetadata Metadata = {});

	/**
	 * Try to create a semantic version object from a string.
	 * If the string cannot be parsed to a valid semantic version,
	 * it's set to a default-constrcuted version.
	 */
	FSemanticVersion(const FString& SourceString, ESemVerParsingStrictness Strictness = ESemVerParsingStrictness::Strict);
	
	UPROPERTY(BlueprintReadWrite)
	int32 MajorVersion = 0;

	UPROPERTY(BlueprintReadWrite)
	int32 MinorVersion = 1;

	UPROPERTY(BlueprintReadWrite)
	int32 PatchVersion = 0;

	UPROPERTY(BlueprintReadWrite)
	FSemVerPreReleaseIdentifier PreReleaseIdentifier;

	UPROPERTY(BlueprintReadWrite)
	FSemVerBuildMetadata BuildMetadata;

	FString ToString() const;

	/**
	 * Try to set this semantic version from a string.
	 * If the string cannot be parsed to a valid semantic version,
	 * it's reset to a default constructed SemVer.
	 * @returns if parsing the semantic version string was successful
	 */
	bool TryParseString(const FString& SourceString, ESemVerParsingStrictness Strictness);

	int32 GetMajorVersion() const;
	int32 GetMinorVersion() const;
	int32 GetPatchVersion() const;

	/**
	 * Increments the major version.
	 * Resets minor and patch version to 0.
	 * Strips pre-release identifier and build metadata.
	 */
	void IncrementMajorVersion();
	
	/**
	 * Increments the minor version.
	 * Resets patch version to 0.
	 * Strips pre-release identifier and build metadata.
	 */
	void IncrementMinorVersion();
	
	/**
	 * Increments the minor version.
	 * Strips pre-release identifier and build metadata.
	 */
	void IncrementPatchVersion();

	/** Increments the pre-release version, if it ends in digits */
	bool TryIncrementPreReleaseVersion();

	/**
	 * @returns Is the precedence of the other version equal to this version.
	 * Ignores the build metadata.
	 */
	bool EqualsPrecedence(const FSemanticVersion& Other) const;

	/**
	 * operator==() checks equality including BuildMetadata.
	 * This means it cannot be used to check precedence!
	 * If you want to check if two semantic versions have the same precedence,
	 * call EqualsPrecedence() instead.
	 */ 
	bool operator==(const FSemanticVersion& Other) const;
	bool operator!=(const FSemanticVersion& Other) const;

	/** Does this version have lower precedence than the other SemVer? */
	bool operator<(const FSemanticVersion& Other) const;
	
	/** Does this version have lower or equal precedence as the other SemVer? */
	bool operator<=(const FSemanticVersion& Other) const;
	
	/** Does this version have higher precedence than the other SemVer? */
	bool operator>(const FSemanticVersion& Other) const;

	/** Does this version have higher or equal precedence as the other SemVer? */
	bool operator>=(const FSemanticVersion& Other) const;

private:
	bool TryParseString_Internal(const FString & SourceString, ESemVerParsingStrictness Strictness);
	bool ComparePrecedence_Internal(const FSemanticVersion& Other, bool bSmallerThan) const;
};
