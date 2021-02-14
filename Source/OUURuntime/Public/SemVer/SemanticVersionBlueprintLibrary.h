// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "SemVer/SemanticVersion.h"
#include "SemanticVersionBlueprintLibrary.generated.h"

/**
 * This blueprint library exposes member functions of SemVer struct types to Blueprint.
 * Any additional functionality that is not present in existing C++ member functions should be placed in other blueprint libraries.
 */
UCLASS()
class OUURUNTIME_API USemanticVersionBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//----------------------
	// FSemanticVersion
	//----------------------

	/**
	 * Try to parse a semantic version object from a source string.
	 * If the string cannot be parsed to a valid semantic version,
	 * it's reset to a default constructed SemVer.
	 * @param SourceString The string that contains the semantic version
	 * @param Strictness How strict the string parsing functions should be adhering to the standard
	 * @param OutSemanticVersion The resulting semantic version object
	 * @returns if parsing was successful
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool TryParseSemVerString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemanticVersion& OutSemanticVersion);

	/** Increment the major version of a semantic version object */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static void IncrementSemVerMajorVersion(UPARAM(ref) FSemanticVersion& Version);

	/** Increment the minor version of a semantic version object */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static void IncrementSemVerMinorVersion(UPARAM(ref) FSemanticVersion& Version);

	/** Increment the patch version of a semantic version object */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static void IncrementSemVerPatchVersion(UPARAM(ref) FSemanticVersion& Version);

	/**
	 * Try to increment the pre-release version of a semantic version object.
	 * Only works if the pre-release version ends in digits.
	 * @returns if incrementing the pre-release version was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool TryIncrementSemVerPreReleaseVersion(UPARAM(ref) FSemanticVersion& Version);

	/**
	 * @returns Is the precedence of the other version equal to this version.
	 * Ignores the build metadata.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool SemVerEqualPrecedence(const FSemanticVersion& A, const FSemanticVersion& B);

	/**
	 * Checks equality including BuildMetadata.
	 * This means it cannot be used to check precedence!
	 * If you want to check if two semantic versions have the same precedence,
	 * call SemVerEqualPrecedence() instead.
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (SemVer)", CompactNodeTitle = "==", ScriptMethod = "Equals", ScriptOperator = "==", Keywords = "== equal"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool Equal_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	/**
	 * Checks in-equality including BuildMetadata.
	 * This means it cannot be used to check precedence!
	 * If you want to check if two semantic versions have the same or different precedence,
	 * call SemanticVersionsEqualPrecedence() instead.
	 */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Not Equal (SemVer)", CompactNodeTitle = "!=", ScriptMethod = "NotEqual", ScriptOperator = "==", Keywords = "== not equal"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool NotEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	/** Does one version have lower precedence than the other SemVer? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer < SemVer", CompactNodeTitle="<", Keywords="< less"), Category="Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool Less_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	/** Does one version have lower or equal precedence as the other SemVer? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer <= SemVer", CompactNodeTitle = "<=", Keywords = "<= less"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool LessEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	/** Does one version have higher precedence than the other SemVer? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer > SemVer", CompactNodeTitle = ">", Keywords = "> greater"), Category="Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool Greater_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	/** Does one version have higher or equal precedence as the other SemVer? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer >= SemVer", CompactNodeTitle = ">=", Keywords = ">= greater"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool GreaterEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	/** Convert a semantic version into its string representation */
	UFUNCTION(BlueprintPure, meta=(DisplayName = "ToString (SemVer)", CompactNodeTitle = "->", Keywords="cast convert", BlueprintAutocast), Category="Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static FString Conv_SemVerString(const FSemanticVersion& InSemanticVersion);

	//----------------------
	// FSemVerPreReleaseIdentifier
	//----------------------

	/**
	* Try to create a pre-release identifier from a string.
	* If the string cannot be parsed to a valid pre-release identifier,
	* the identifier remains empty.
	* @param SourceString The string from which the semver will be constructed
	* @param Strictness How strict the string parsing functions should be adhering to the standard
	* @param OutReleaseIdentifier The resulting pre-release identifier
	* @returns if parsing was successful
	*/
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool TryParseSemVerPreReleaseIdentifierString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemVerPreReleaseIdentifier& OutReleaseIdentifier);

	/**
	 * Try to increment a pre-release identifier. Only works if the last identifier has only digits or is empty.
	 * @returns if incrementing the pre-release was successful
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	bool TryIncrementSemVerPreReleaseIdentifier(UPARAM(ref) FSemVerPreReleaseIdentifier& PreReleaseIdentifier);

	/** Does one pre-release identifier have the same value and precedence as the other one? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (SemVer PreRelease)", CompactNodeTitle = "==", ScriptMethod = "Equals", ScriptOperator = "==", Keywords = "== equal"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool Equal_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	/** Does one pre-release identifier have differing value or precedence as the other one? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Not Equal (SemVer PreRelease)", CompactNodeTitle = "!=", ScriptMethod = "NotEqual", ScriptOperator = "==", Keywords = "== not equal"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool NotEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	/** Does one pre-release identifier have lower precedence than the other one? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease < SemVer PreRelease", CompactNodeTitle="<", Keywords="< less"), Category="Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool Less_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	/** Does one pre-release identifier have lower or equal precedence as the other one? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease <= SemVer PreRelease", CompactNodeTitle = "<=", Keywords = "<= less"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool LessEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	/** Does one pre-release identifier have higher precedence than the other one? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease > SemVer PreRelease", CompactNodeTitle = ">", Keywords = "> greater"), Category="Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool Greater_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	/** Does one pre-release identifier have higher or equal precedence as the other one? */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease >= SemVer PreRelease", CompactNodeTitle = ">=", Keywords = ">= greater"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool GreaterEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	/** Convert a pre-release identifier to its string representation */
	UFUNCTION(BlueprintPure, meta=(DisplayName = "ToString (SemVer PreRelease)", CompactNodeTitle = "->", Keywords="cast convert", BlueprintAutocast), Category="Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static FString Conv_PreReleaseString(const FSemVerPreReleaseIdentifier& InReleaseIdentifier);

	//----------------------
	// FSemVerBuildMetadata
	//----------------------

	/**
	 * Try to create a metadata object from a string.
	 * If the string cannot be parsed to a valid metadata object,
	 * the object remains empty.
	 * @param SourceString The string from which the build metadata is constructed
	 * @param Strictness How strict the string parsing functions should be adhering to the standard
	 * @param OutBuildMetadata The resulting build metadata
	 * @returns if parsing was successful
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Build Metadata")
	static bool TryParseSemVerBuildMetadataString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemVerBuildMetadata& OutBuildMetadata);

	/** If one build metadata equals another build metadata */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (SemVer BuildMetadata)", CompactNodeTitle = "==", ScriptMethod = "Equals", ScriptOperator = "==", Keywords = "== equal"), Category = "Open Unreal Utilities|Semantic Versioning|Build Metadata")
	static bool Equal_BuildMetadataBuildMetadata(const FSemVerBuildMetadata& A, const FSemVerBuildMetadata& B);

	/** If one build metadata does not equal another build metadata */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Not Equal (SemVer BuildMetadata)", CompactNodeTitle = "!=", ScriptMethod = "NotEqual", ScriptOperator = "==", Keywords = "== not equal"), Category = "Open Unreal Utilities|Semantic Versioning|Build Metadata")
	static bool NotEqual_BuildMetadataBuildMetadata(const FSemVerBuildMetadata& A, const FSemVerBuildMetadata& B);

	/** Convert a build metadata object to its string representation */
	UFUNCTION(BlueprintPure, meta=(DisplayName = "ToString (SemVer BuildMetadata)", CompactNodeTitle = "->", Keywords="cast convert", BlueprintAutocast), Category="Open Unreal Utilities|Semantic Versioning|Build Metadata")
	static FString Conv_BuildMetadataString(const FSemVerBuildMetadata& InBuildMetadata);
};
