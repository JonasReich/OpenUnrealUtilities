// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "SemVer/SemVerParsingStrictness.h"

#include "BuildMetadata.generated.h"

//////////////////////////////////////////////////////////////////////////
// The types defined in this file and all other in this directory implement
// Semantic Versioning 2.0.0 as specified at https://semver.org/spec/v2.0.0.html
//////////////////////////////////////////////////////////////////////////

/**
 * The build metadata part of a semantic version
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FSemVerBuildMetadata
{
	GENERATED_BODY()
public:
	FSemVerBuildMetadata() = default;

	/**
	 * Try to create a metadata object from a string.
	 * If the string cannot be parsed to a valid metadata object,
	 * the object remains empty.
	 */
	FSemVerBuildMetadata(
		const FString& SourceString,
		ESemVerParsingStrictness InStrictness = ESemVerParsingStrictness::Strict);

	FString ToString() const;

	/**
	 * Try to set this metadata object from a string.
	 * If the string cannot be parsed to a valid metadata object,
	 * the build metadata is reset to empty metadata.
	 * @returns if parsing the metadata string was successful
	 */
	bool TryParseString(const FString& SourceString, ESemVerParsingStrictness InStrictness);

	bool operator==(const FSemVerBuildMetadata& Other) const;
	bool operator!=(const FSemVerBuildMetadata& Other) const;

protected:
	UPROPERTY(BlueprintReadOnly,Category="OUU|Runtime|SemVer")
	FString Metadata;

	/** How strictly this metadata adheres to the semver specification */
	UPROPERTY(BlueprintReadOnly,Category="OUU|Runtime|SemVer")
	ESemVerParsingStrictness Strictness = ESemVerParsingStrictness::Strict;
};
