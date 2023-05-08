// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Misc/RegexUtils.h"

BEGIN_DEFINE_SPEC(FRegexUtilsSpec, "OpenUnrealUtilities.Runtime.Misc.Regex", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FRegexUtilsSpec)
void FRegexUtilsSpec::Define()
{
	Describe("MatchesRegex", [this]() {
		It("should return true if the regex is an exact match",
		   [this]() { SPEC_TEST_TRUE(OUU::Runtime::RegexUtils::MatchesRegex("[a-z]*", "alphabet")); });

		It("should return true if the test string contains a pattern match and some additional content",
		   [this]() { SPEC_TEST_TRUE(OUU::Runtime::RegexUtils::MatchesRegex("[a-z]*", "1234alphabet1234")); });

		It("should return false if the test string does not contain a pattern match",
		   [this]() { SPEC_TEST_FALSE(OUU::Runtime::RegexUtils::MatchesRegex("[a-z]*", "1234")); });

		It("should return false if the test string has length 0",
		   [this]() { SPEC_TEST_FALSE(OUU::Runtime::RegexUtils::MatchesRegex("[a-z]*", "")); });
	});

	Describe("MatchesRegexExact", [this]() {
		It("should return true if the regex is an exact match",
		   [this]() { SPEC_TEST_TRUE(OUU::Runtime::RegexUtils::MatchesRegexExact("[a-z]*", "alphabet")); });

		It("should return false if the test string contains a pattern match and some additional content",
		   [this]() { SPEC_TEST_FALSE(OUU::Runtime::RegexUtils::MatchesRegexExact("[a-z]*", "1234alphabet1234")); });

		It("should return false if the test string has length 0",
		   [this]() { SPEC_TEST_FALSE(OUU::Runtime::RegexUtils::MatchesRegexExact("[a-z]*", "")); });

		It("should be the same as using MatchesRegex and ^$ as line start/end delimiters", [this]() {
			SPEC_TEST_FALSE(OUU::Runtime::RegexUtils::MatchesRegexExact("[a-z]*", "1234alphabet1234"));
			SPEC_TEST_FALSE(OUU::Runtime::RegexUtils::MatchesRegex("^[a-z]*$", "1234alphabet1234"));
			SPEC_TEST_TRUE(OUU::Runtime::RegexUtils::MatchesRegex("[a-z]*", "1234alphabet1234"));
			SPEC_TEST_TRUE(OUU::Runtime::RegexUtils::MatchesRegex("^[a-z]$*", "alphabet"));
		});
	});

	Describe("CountRegexMatches", [this]() {
		It("should return 0 if no matches were found",
		   [this]() { SPEC_TEST_EQUAL(OUU::Runtime::RegexUtils::CountRegexMatches("[a-z]*", "1234"), 0); });

		It("should return 0 if the test string has length 0",
		   [this]() { SPEC_TEST_EQUAL(OUU::Runtime::RegexUtils::CountRegexMatches("[a-z]*", ""), 0); });

		It("should return the count of matches if there are multiple matches", [this]() {
			SPEC_TEST_EQUAL(OUU::Runtime::RegexUtils::CountRegexMatches("[a-z]*", "alphabet 1234 noodle soup"), 3);
		});
	});

	Describe("GetRegexMatches", [this]() {
		It("should return an empty list if no matches were found", [this]() {
			auto Matches = OUU::Runtime::RegexUtils::GetRegexMatches("[a-z]*", "1234");
			SPEC_TEST_ARRAYS_EQUAL(Matches, {});
		});

		It("should return a list of all matches when one or more matches were found", [this]() {
			TArray<FRegexMatch> ExpectedMatches = {
				FRegexMatch{0, 8, "alphabet"},
				FRegexMatch{14, 20, "noodle"},
				FRegexMatch{21, 25, "soup"},
			};
			auto Matches = OUU::Runtime::RegexUtils::GetRegexMatches("[a-z]*", "alphabet 1234 noodle soup");
			SPEC_TEST_ARRAYS_EQUAL(Matches, ExpectedMatches);
		});
	});

	Describe("GetRegexMatchesAndGroups", [this]() {
		It("should return an empty list if no matches were found", [this]() {
			auto Matches = OUU::Runtime::RegexUtils::GetRegexMatches("[a-z]*", "1234");
			SPEC_TEST_ARRAYS_EQUAL(Matches, {});
		});

		Describe("returns a list of all matches and groups when one or more matches were found", [this]() {
			TArray<TTuple<FString, TArray<FRegexGroups>>> TestCases = {
				TTuple<FString, TArray<FRegexGroups>>{
					"unrealengine.com",
					TArray<FRegexGroups>{FRegexGroups{
						{FRegexMatch{0, 16, "unrealengine.com"},
						 FRegexMatch{0, 12, "unrealengine"},
						 FRegexMatch{13, 16, "com"}}}}},
				TTuple<FString, TArray<FRegexGroups>>{
					"epicgames.com",
					TArray<FRegexGroups>{FRegexGroups{
						{FRegexMatch{0, 13, "epicgames.com"},
						 FRegexMatch{0, 9, "epicgames"},
						 FRegexMatch{10, 13, "com"}}}}},
				TTuple<FString, TArray<FRegexGroups>>{
					"jonasreich.de",
					TArray<FRegexGroups>{FRegexGroups{
						{FRegexMatch{0, 13, "jonasreich.de"},
						 FRegexMatch{0, 10, "jonasreich"},
						 FRegexMatch{11, 13, "de"}}}}},
				TTuple<FString, TArray<FRegexGroups>>{
					"xx@asdf-12378 jonasreich.de, 123 epicgames.com open-unreal-utilities",
					TArray<FRegexGroups>{
						FRegexGroups{
							{FRegexMatch{14, 27, "jonasreich.de"},
							 FRegexMatch{14, 24, "jonasreich"},
							 FRegexMatch{25, 27, "de"}}},
						FRegexGroups{
							{FRegexMatch{33, 46, "epicgames.com"},
							 FRegexMatch{33, 42, "epicgames"},
							 FRegexMatch{43, 46, "com"}}}}}};

			for (TTuple<FString, TArray<FRegexGroups>> TestCase : TestCases)
			{
				It(*FString::Printf(TEXT("should parse '%s' correctly"), *EscapeTestName(TestCase.Get<0>())),
				   [this, TestCase]() {
					   FString Pattern = "([\\w-]+)\\.(\\w+)";
					   FString TestString = TestCase.Get<0>();
					   TArray<FRegexGroups> ExpectedResult = TestCase.Get<1>();
					   auto MatchesAndGroups =
						   OUU::Runtime::RegexUtils::GetRegexMatchesAndGroups(Pattern, 2, TestString);
					   SPEC_TEST_ARRAYS_EQUAL(MatchesAndGroups, ExpectedResult);
				   });
			}
		});
	});

	Describe("ReplaceFirstRegexMatch", [this]() {
		It("should replace a single occurence with a given string", [this]() {
			const FString Input = "My test string";
			const FString Result = OUU::Runtime::RegexUtils::ReplaceFirstRegexMatch("t.{2}t", Input, "foo");
			SPEC_TEST_EQUAL(Result, "My foo string");
		});

		It("should replace a single occurence with a given string (even when longer than input)", [this]() {
			const FString Input = "My test string";
			const FString Result = OUU::Runtime::RegexUtils::ReplaceFirstRegexMatch("t.{2}t", Input, "foobar");
			SPEC_TEST_EQUAL(Result, "My foobar string");
		});

		It("should only replace the first occurence", [this]() {
			const FString Input = "My test string test test";
			const FString Result = OUU::Runtime::RegexUtils::ReplaceFirstRegexMatch("t.{2}t", Input, "foobar");
			SPEC_TEST_EQUAL(Result, "My foobar string test test");
		});
	});

	Describe("ReplaceAllRegexMatches", [this]() {
		It("should replace all occurrences", [this]() {
			const FString Input = "My test string test test";
			const FString Result = OUU::Runtime::RegexUtils::ReplaceAllRegexMatches("t.{2}t", Input, "foobar");
			SPEC_TEST_EQUAL(Result, "My foobar string foobar foobar");
		});

		It("should replace all occurrences even when first match starts at first character", [this]() {
			const FString Input = "test My test string test test";
			const FString Result = OUU::Runtime::RegexUtils::ReplaceAllRegexMatches("t.{2}t", Input, "foobar");
			SPEC_TEST_EQUAL(Result, "foobar My foobar string foobar foobar");
		});
	});
}

#endif
