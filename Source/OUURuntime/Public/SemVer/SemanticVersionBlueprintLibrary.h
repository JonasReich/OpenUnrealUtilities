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
	
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool TryParseSemanticVersionString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemanticVersion& OutSemanticVersion);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static void IncrementSemanticMajorVersion(UPARAM(ref) FSemanticVersion& Version);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static void IncrementSemanticMinorVersion(UPARAM(ref) FSemanticVersion& Version);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static void IncrementSemanticPatchVersion(UPARAM(ref) FSemanticVersion& Version);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool TryIncrementSemanticPreReleaseVersion(UPARAM(ref) FSemanticVersion& Version);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool SemanticVersionsEqualPrecedence(const FSemanticVersion& A, const FSemanticVersion& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (SemVer)", CompactNodeTitle = "==", ScriptMethod = "Equals", ScriptOperator = "==", Keywords = "== equal"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool Equal_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Not Equal (SemVer)", CompactNodeTitle = "!=", ScriptMethod = "NotEqual", ScriptOperator = "==", Keywords = "== not equal"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool NotEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer < SemVer", CompactNodeTitle="<", Keywords="< less"), Category="Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool Less_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer <= SemVer", CompactNodeTitle = "<=", Keywords = "<= less"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
	static bool LessEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer > SemVer", CompactNodeTitle = ">", Keywords = "> greater"), Category="Open Unreal Utilities|Semantic Versioning|Semantic Version")
    static bool Greater_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer >= SemVer", CompactNodeTitle = ">=", Keywords = ">= greater"), Category = "Open Unreal Utilities|Semantic Versioning|Semantic Version")
    static bool GreaterEqual_SemVerSemVer(const FSemanticVersion& A, const FSemanticVersion& B);

	UFUNCTION(BlueprintPure, meta=(DisplayName = "ToString (SemVer)", CompactNodeTitle = "->", Keywords="cast convert", BlueprintAutocast), Category="Open Unreal Utilities|Semantic Versioning|Semantic Version")
    static FString Conv_SemVerString(const FSemanticVersion& InSemanticVersion);

	//----------------------
	// FSemVerPreReleaseIdentifier
	//----------------------

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool TryParseSemanticVersionReleaseIdentifierString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemVerPreReleaseIdentifier& OutReleaseIdentifier);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	bool TryIncrementSemanticVersionReleaseIdentifier(UPARAM(ref) FSemVerPreReleaseIdentifier& PreReleaseIdentifier);
	
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (SemVer PreRelease)", CompactNodeTitle = "==", ScriptMethod = "Equals", ScriptOperator = "==", Keywords = "== equal"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool Equal_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Not Equal (SemVer PreRelease)", CompactNodeTitle = "!=", ScriptMethod = "NotEqual", ScriptOperator = "==", Keywords = "== not equal"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool NotEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease < SemVer PreRelease", CompactNodeTitle="<", Keywords="< less"), Category="Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool Less_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease <= SemVer PreRelease", CompactNodeTitle = "<=", Keywords = "<= less"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
	static bool LessEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease > SemVer PreRelease", CompactNodeTitle = ">", Keywords = "> greater"), Category="Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
    static bool Greater_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "SemVer PreRelease >= SemVer PreRelease", CompactNodeTitle = ">=", Keywords = ">= greater"), Category = "Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
    static bool GreaterEqual_PreReleasePreRelease(const FSemVerPreReleaseIdentifier& A, const FSemVerPreReleaseIdentifier& B);

	UFUNCTION(BlueprintPure, meta=(DisplayName = "ToString (SemVer PreRelease)", CompactNodeTitle = "->", Keywords="cast convert", BlueprintAutocast), Category="Open Unreal Utilities|Semantic Versioning|Pre-Release Identifier")
    static FString Conv_PreReleaseString(const FSemVerPreReleaseIdentifier& InReleaseIdentifier);

	//----------------------
	// FSemVerBuildMetadata
	//----------------------

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning|Build Metadata")
    static bool TryParseSemVerBuildMetadataString(const FString& SourceString, ESemVerParsingStrictness Strictness, FSemVerBuildMetadata& OutBuildMetadata);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Equal (SemVer BuildMetadata)", CompactNodeTitle = "==", ScriptMethod = "Equals", ScriptOperator = "==", Keywords = "== equal"), Category = "Open Unreal Utilities|Semantic Versioning|Build Metadata")
    static bool Equal_BuildMetadataBuildMetadata(const FSemVerBuildMetadata& A, const FSemVerBuildMetadata& B);

	UFUNCTION(BlueprintPure, meta = (DisplayName = "Not Equal (SemVer BuildMetadata)", CompactNodeTitle = "!=", ScriptMethod = "NotEqual", ScriptOperator = "==", Keywords = "== not equal"), Category = "Open Unreal Utilities|Semantic Versioning|Build Metadata")
    static bool NotEqual_BuildMetadataBuildMetadata(const FSemVerBuildMetadata& A, const FSemVerBuildMetadata& B);

	UFUNCTION(BlueprintPure, meta=(DisplayName = "ToString (SemVer BuildMetadata)", CompactNodeTitle = "->", Keywords="cast convert", BlueprintAutocast), Category="Open Unreal Utilities|Semantic Versioning|Build Metadata")
    static FString Conv_BuildMetadataString(const FSemVerBuildMetadata& InBuildMetadata);
};
