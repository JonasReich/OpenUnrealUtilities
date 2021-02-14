// Copyright (c) 2021 Jonas Reich

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
	FRegexMatch(int32 Beginning, int32 Ending, FString InMatchString) :
		MatchBeginning(Beginning),
		MatchEnding(Ending),
		MatchString(InMatchString)
	{}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MatchBeginning;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	int32 MatchEnding;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FString MatchString;

	FORCEINLINE FString ToString() const
	{
		return FString::Printf(TEXT("(%i-%i:%s)"), MatchBeginning, MatchEnding, *MatchString);
	}

	FORCEINLINE bool operator==(const FRegexMatch& Other) const
	{
		return MatchBeginning == Other.MatchBeginning
			&& MatchEnding == Other.MatchEnding 
			&& MatchString == Other.MatchString;
	}

	FORCEINLINE bool operator!=(const FRegexMatch& Other) const
	{
		return !(*this == Other);
	}
};

/** A complex regex result including the match and all of the capture groups */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FRegexGroups
{
	GENERATED_BODY()
public:
	FRegexGroups() = default;
	FRegexGroups(TArray<FRegexMatch> InCaptureGroups) : CaptureGroups(InCaptureGroups) {}
	FRegexGroups(FRegexMatch InMatch) : CaptureGroups({ InMatch }) {}

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	TArray<FRegexMatch> CaptureGroups;

	FORCEINLINE FString ToString() const
	{
		return ArrayToString(CaptureGroups);
	}

	FORCEINLINE bool operator==(const FRegexGroups& Other) const
	{
		return CaptureGroups == Other.CaptureGroups;
	}

	FORCEINLINE bool operator!=(const FRegexGroups& Other) const
	{
		return !(*this == Other);
	}

	bool IsValid() const
	{
		return CaptureGroups.Num() > 0;
	}
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
	static TArray<FRegexGroups> GetRegexMatchesAndGroups(const FString& RegexPattern, int32 GroupCount, const FString& TestString);

	/** @returns the single regex match of the pattern in the test string together with all of the capture groups,
	 * IF the match ranges from the beginning to the end of the test string (all characters included). */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Regex")
	static FRegexGroups GetRegexMatchAndGroupsExact(const FString& RegexPattern, int32 GroupCount, const FString& TestString);
};

using FRegexUtils = URegexFunctionLibrary;
