// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "SemVer/SemVerParsingStrictness.h"
#include "PreReleaseIdentifier.generated.h"

//////////////////////////////////////////////////////////////////////////
// The types defined in this file and all other in this directory implement 
// Semantic Versioning 2.0.0 as specified at https://semver.org/spec/v2.0.0.html
//////////////////////////////////////////////////////////////////////////

/**
 * The pre-release identifier of a semantic version.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FSemVerPreReleaseIdentifier
{
	GENERATED_BODY()
public:
	FSemVerPreReleaseIdentifier() = default;

	/**
	 * Try to create a pre-release identifier from a string.
	 * If the string cannot be parsed to a valid pre-release identifier,
	 * the identifier remains empty.
	 */
	FSemVerPreReleaseIdentifier(const FString& SourceString, ESemVerParsingStrictness InStrictness = ESemVerParsingStrictness::Strict);

	FString ToString() const;

	/**
	 * Try to set this pre-release version from a string.
	 * If the string cannot be parsed to a valid pre-release identifier,
	 * the identifier is reset to an empty identifier.
	 * @returns if parsing the source string was successful
	 */
	bool TryParseString(const FString& SourceString, ESemVerParsingStrictness InStrictness);

	/** @returns the dot-separated identifiers that comprise the combined pre-release identifier */
	const TArray<FString> GetIdentifiers();

	/**
	 * Try to increment this pre-release identifier. Only works if the last identifier has only digits or is empty.
	 * @returns if incrementing the pre-release was successful.
	 */
	bool TryIncrement();

	/** Does this pre-release identifier have the same value and precedence as the other one? */
	bool operator==(const FSemVerPreReleaseIdentifier& Other) const;

	/** Does this pre-release identifier have differing value or precedence as the other one? */
	bool operator!=(const FSemVerPreReleaseIdentifier& Other) const;

	/** Does this pre-release identifier have lower precedence than the other one? */
	bool operator<(const FSemVerPreReleaseIdentifier& Other) const;

	/** Does this pre-release identifier have lower or equal precedence as the other one? */
	bool operator<=(const FSemVerPreReleaseIdentifier& Other) const;

	/** Does this pre-release identifier have higher precedence than the other one? */
	bool operator>(const FSemVerPreReleaseIdentifier& Other) const;

	/** Does this pre-release identifier have higher or equal precedence as the other one? */
	bool operator>=(const FSemVerPreReleaseIdentifier& Other) const;

protected:
	UPROPERTY(BlueprintReadOnly)
	TArray<FString> Identifiers;

	/** How strictly this pre-release identifier adheres to the semver specification */
	UPROPERTY(BlueprintReadOnly)
	ESemVerParsingStrictness Strictness = ESemVerParsingStrictness::Strict;

	static bool CompareStringIdentifiersSmaller(const FString& A, const FString& B);

	static bool CompareStringIdentifiersBigger(const FString& A, const FString& B);

	static int32 TryParseNumericIdentifier(const FString& Identifier);
};
