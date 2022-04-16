// Copyright (c) 2022 Jonas Reich

#include "Misc/RegexUtils.h"

#include "Internationalization/Regex.h"

namespace OUU::Runtime::Private::Regex
{
	struct FScopedRegex
	{
		FRegexPattern Pattern;
		FRegexMatcher Matcher;

		FScopedRegex(const FString& PatternString, const FString& InputString) :
			Pattern(PatternString), Matcher(Pattern, InputString)
		{
		}

		FRegexMatcher* operator->() { return &Matcher; }
	};

	bool IsExactMatchRange(const FString& TestString, int32 MatchBeginning, int32 MatchEnding)
	{
		int32 StringLength = TestString.Len();
		return (StringLength > 0) && (MatchBeginning == 0) && (MatchEnding == StringLength);
	}

	bool IsValidMatchRange(const FString& TestString, int32 MatchBeginning, int32 MatchEnding)
	{
		int32 StringLength = TestString.Len();
		return (MatchEnding > MatchBeginning) && StringLength > 0
			&& (MatchBeginning >= 0 && MatchBeginning <= StringLength)
			&& (MatchEnding >= 0 && MatchEnding <= StringLength);
	}

	template <typename ResultType, typename TruePredicateType, typename FalsePredicateType>
	ResultType MatchRegex(
		const FString& PatternString,
		const FString& TestString,
		int32 BeginIndex,
		TruePredicateType TruePredicate,
		FalsePredicateType FalsePredicate)
	{
		FScopedRegex Regex{PatternString, TestString};
		int32 StringLength = TestString.Len();
		for (int32 i = BeginIndex; i < StringLength; i++)
		{
			Regex->SetLimits(i, StringLength);
			if (!Regex->FindNext())
				continue;

			int32 MatchBeginning = Regex->GetMatchBeginning();
			int32 MatchEnding = Regex->GetMatchEnding();
			if (IsValidMatchRange(TestString, MatchBeginning, MatchEnding))
			{
				return TruePredicate(Regex, MatchBeginning, MatchEnding, MatchEnding);
			}
		}
		return FalsePredicate();
	}

	bool MatchesRegex_Recursive(const FString& RegexPattern, const FString& TestString, int32 BeginIndex)
	{
		return MatchRegex<bool>(
			RegexPattern,
			TestString,
			BeginIndex,
			[&](FScopedRegex& Regex, int32 MatchBeginning, int32 MatchEnding, int32 BeginIndex) { return true; },
			[]() { return false; });
	}

	int32 CountRegexMatches_Recursive(const FString& RegexPattern, const FString& TestString, int32 BeginIndex)
	{
		return MatchRegex<int32>(
			RegexPattern,
			TestString,
			BeginIndex,
			[&](FScopedRegex& Regex, int32 MatchBeginning, int32 MatchEnding, int32 BeginIndex) {
				return CountRegexMatches_Recursive(RegexPattern, TestString, BeginIndex) + 1;
			},
			[]() { return 0; });
	}

	TArray<FRegexMatch> GetRegexMatches_Recursive(
		const FString& RegexPattern,
		const FString& TestString,
		int32 BeginIndex)
	{
		return MatchRegex<TArray<FRegexMatch>>(
			RegexPattern,
			TestString,
			BeginIndex,
			[&](FScopedRegex& Regex, int32 MatchBeginning, int32 MatchEnding, int32 BeginIndex) {
				TArray<FRegexMatch> ResultList;
				ResultList.Add(FRegexMatch{
					MatchBeginning,
					MatchEnding,
					TestString.Mid(MatchBeginning, MatchEnding - MatchBeginning)});
				ResultList.Append(GetRegexMatches_Recursive(RegexPattern, TestString, BeginIndex));
				return ResultList;
			},
			[]() { return TArray<FRegexMatch>{}; });
	}

	TArray<FRegexGroups> GetRegexMatchesAndGroups_Recursive(
		const FString& RegexPattern,
		int32 GroupCount,
		const FString& TestString,
		int32 BeginIndex)
	{
		return MatchRegex<TArray<FRegexGroups>>(
			RegexPattern,
			TestString,
			BeginIndex,
			[&](FScopedRegex& Regex, int32 MatchBeginning, int32 MatchEnding, int32 BeginIndex) {
				TArray<FRegexGroups> ResultList;
				FRegexGroups Result;
				for (int32 i = 0; i < GroupCount; i++)
				{
					Result.CaptureGroups.Add(FRegexMatch{
						Regex->GetCaptureGroupBeginning(i),
						Regex->GetCaptureGroupEnding(i),
						Regex->GetCaptureGroup(i)});
				}
				ResultList.Add(Result);
				ResultList.Append(GetRegexMatchesAndGroups_Recursive(RegexPattern, GroupCount, TestString, BeginIndex));
				return ResultList;
			},
			[]() { return TArray<FRegexGroups>{}; });
	}

} // namespace OUU::Runtime::Private::Regex

bool URegexFunctionLibrary::MatchesRegex(const FString& RegexPattern, const FString& TestString)
{
	return OUU::Runtime::Private::Regex::MatchesRegex_Recursive(RegexPattern, TestString, 0);
}

int32 URegexFunctionLibrary::CountRegexMatches(const FString& RegexPattern, const FString& TestString)
{
	return OUU::Runtime::Private::Regex::CountRegexMatches_Recursive(RegexPattern, TestString, 0);
}

bool URegexFunctionLibrary::MatchesRegexExact(const FString& RegexPattern, const FString& TestString)
{
	OUU::Runtime::Private::Regex::FScopedRegex Regex(RegexPattern, TestString);
	if (!Regex->FindNext())
		return false;

	return OUU::Runtime::Private::Regex::IsExactMatchRange(
		TestString,
		Regex->GetMatchBeginning(),
		Regex->GetMatchEnding());
}

TArray<FRegexMatch> URegexFunctionLibrary::GetRegexMatches(const FString& RegexPattern, const FString& TestString)
{
	return OUU::Runtime::Private::Regex::GetRegexMatches_Recursive(RegexPattern, TestString, 0);
}

FRegexGroups URegexFunctionLibrary::GetRegexMatchAndGroupsExact(
	const FString& RegexPattern,
	int32 GroupCount,
	const FString& TestString)
{
	FRegexGroups Result;

	OUU::Runtime::Private::Regex::FScopedRegex Regex{RegexPattern, TestString};
	int32 StringLength = TestString.Len();
	if (!Regex->FindNext())
		return Result;

	int32 MatchBeginning = Regex->GetMatchBeginning();
	int32 MatchEnding = Regex->GetMatchEnding();
	if (!OUU::Runtime::Private::Regex::IsExactMatchRange(TestString, MatchBeginning, MatchEnding))
		return Result;

	TArray<FRegexGroups> ResultList;
	for (int32 i = 0; i < (GroupCount + 1); i++)
	{
		Result.CaptureGroups.Add(FRegexMatch{
			Regex->GetCaptureGroupBeginning(i),
			Regex->GetCaptureGroupEnding(i),
			Regex->GetCaptureGroup(i)});
	}

	return Result;
}

TArray<FRegexGroups> URegexFunctionLibrary::GetRegexMatchesAndGroups(
	const FString& RegexPattern,
	int32 GroupCount,
	const FString& TestString)
{
	return OUU::Runtime::Private::Regex::GetRegexMatchesAndGroups_Recursive(
		RegexPattern,
		GroupCount + 1,
		TestString,
		0);
}

FRegexGroups URegexFunctionLibrary::GetFirstRegexMatchAndGroups(
	const FString& RegexPattern,
	int32 GroupCount,
	const FString& TestString)
{
	auto Result = GetRegexMatchesAndGroups(RegexPattern, GroupCount, TestString);
	return Result.Num() > 0 ? Result[0] : FRegexGroups::Invalid();
}

FString URegexFunctionLibrary::ReplaceFirstRegexMatch(
	const FString& RegexPattern,
	const FString& InputString,
	const FString& ReplaceString)
{
	auto Matches = GetRegexMatches(RegexPattern, InputString);
	if (Matches.Num() < 1)
		return InputString;
	const auto& FirstMatch = Matches[0];
	return InputString.Mid(0, FirstMatch.MatchBeginning) + ReplaceString + InputString.Mid(FirstMatch.MatchEnding);
}

FString URegexFunctionLibrary::ReplaceAllRegexMatches(
	const FString& RegexPattern,
	const FString& InputString,
	const FString& ReplaceString)
{
	auto Matches = GetRegexMatches(RegexPattern, InputString);
	int32 LastMatchEnding = 0;
	FString Result = "";
	for (const auto& Match : Matches)
	{
		// All the chars from input string between last match end and new match
		const FString CarryOver = InputString.Mid(LastMatchEnding, Match.MatchBeginning - LastMatchEnding);

		Result += CarryOver + ReplaceString;
		LastMatchEnding = Match.MatchEnding;
	}
	// Rest of the string until end
	Result += InputString.Mid(LastMatchEnding);
	return Result;
}
