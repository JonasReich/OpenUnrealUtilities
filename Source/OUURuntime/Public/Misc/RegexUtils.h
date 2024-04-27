// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/StringUtils.h"

#include "RegexUtils.generated.h"

class FRegexMatcher;

/** A single regex match */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FRegexMatch
{
	GENERATED_BODY()
public:
	FRegexMatch() = default;

	FRegexMatch(int32 Beginning, int32 Ending, const FString& InMatchString) :
		MatchBeginning(Beginning), MatchEnding(Ending), MatchString(InMatchString)
	{
	}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MatchBeginning = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MatchEnding = INDEX_NONE;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString MatchString = "";

	FORCEINLINE FString ToString() const
	{
		return FString::Printf(TEXT("(%i-%i:%s)"), MatchBeginning, MatchEnding, *MatchString);
	}

	FORCEINLINE bool operator==(const FRegexMatch& Other) const
	{
		return MatchBeginning == Other.MatchBeginning && MatchEnding == Other.MatchEnding
			&& MatchString == Other.MatchString;
	}

	FORCEINLINE bool operator!=(const FRegexMatch& Other) const { return !(*this == Other); }
};

/** A complex regex result including the match and all of the capture groups */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FRegexGroups
{
	GENERATED_BODY()
public:
	FRegexGroups() = default;

	FRegexGroups(const TArray<FRegexMatch>& InCaptureGroups) : CaptureGroups(InCaptureGroups) {}

	FRegexGroups(FRegexMatch InMatch) : CaptureGroups({InMatch}) {}

	static FRegexGroups Invalid() { return FRegexGroups(); }

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FRegexMatch> CaptureGroups;

	FORCEINLINE FString ToString() const { return ArrayToString(CaptureGroups); }

	FORCEINLINE bool operator==(const FRegexGroups& Other) const { return CaptureGroups == Other.CaptureGroups; }

	FORCEINLINE bool operator!=(const FRegexGroups& Other) const { return !(*this == Other); }

	bool IsValid() const { return CaptureGroups.Num() > 0; }
};

/** Simplified and blueprint exposed functions for regex matching. */
UCLASS()
class OUURUNTIME_API URegexFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** @returns if the pattern matches the test string at least once */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static bool MatchesRegex(const FString& RegexPattern, const FString& TestString);

	/** @returns if there is a single match from test string beginning to test string end (all characters included). */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static bool MatchesRegexExact(const FString& RegexPattern, const FString& TestString);

	/** @returns how often the pattern was found in the test string */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static int32 CountRegexMatches(const FString& RegexPattern, const FString& TestString);

	/** @returns all of the matches of the pattern in the test string */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static TArray<FRegexMatch> GetRegexMatches(const FString& RegexPattern, const FString& TestString);

	/** @returns all of the matches of the pattern in the test string together with all of the capture groups */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static TArray<FRegexGroups> GetRegexMatchesAndGroups(
		const FString& RegexPattern,
		int32 GroupCount,
		const FString& TestString);

	/** @returns first of the matches of the pattern in the test string together with all of the capture groups */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static FRegexGroups GetFirstRegexMatchAndGroups(
		const FString& RegexPattern,
		int32 GroupCount,
		const FString& TestString);

	/**
	 *	@returns	the single regex match of the pattern in the test string together with all of the capture groups,
	 *				IF the match ranges from the beginning to the end of the test string (all characters included).
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static FRegexGroups GetRegexMatchAndGroupsExact(
		const FString& RegexPattern,
		int32 GroupCount,
		const FString& TestString);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static FString ReplaceFirstRegexMatch(
		const FString& RegexPattern,
		const FString& InputString,
		const FString& ReplaceString);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static FString ReplaceAllRegexMatches(
		const FString& RegexPattern,
		const FString& InputString,
		const FString& ReplaceString);
};

namespace OUU::Runtime
{
	using RegexUtils = URegexFunctionLibrary;
} // namespace OUU::Runtime

class UE_DEPRECATED(5.0, "FRegexUtils has been deprecated in favor of OUU::Runtime::RegexUtils.") FRegexUtils :
	public OUU::Runtime::RegexUtils
{
};
