// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Core/OUULexicalOperatorLibrary.h"

BEGIN_DEFINE_SPEC(
	FOUULexicalOperatorLibrarySpec,
	"OpenUnrealUtilities.BlueprintRuntime.Core.OUULexicalOperatorLibrary",
	DEFAULT_OUU_TEST_FLAGS)
	template <typename TargetType>
	using TOperatorType = TFunction<bool(const TargetType&, const TargetType&)>;

	template <typename TargetType>
	void DescribeSingleOperatorTestCases(
		TargetType A,
		TargetType B,
		TOperatorType<TargetType> OperatorFunc,
		bool AB_Result,
		bool AA_Result,
		bool BA_Result)
	{
		It(FString::Printf(
			   TEXT("should return %s for strings A (%s) and B (%s)"),
			   *LexToString(AB_Result),
			   *LexToString(A),
			   *LexToString(B)),
		   [=, this]() { SPEC_TEST_EQUAL(OperatorFunc(A, B), AB_Result); });
		It(FString::Printf(
			   TEXT("should return %s for strings A (%s) and A (%s)"),
			   *LexToString(AA_Result),
			   *LexToString(A),
			   *LexToString(A)),
		   [=, this]() { SPEC_TEST_EQUAL(OperatorFunc(A, A), AA_Result); });
		It(FString::Printf(
			   TEXT("should return %s for strings B (%s) and A (%s)"),
			   *LexToString(BA_Result),
			   *LexToString(B),
			   *LexToString(A)),
		   [=, this]() { SPEC_TEST_EQUAL(OperatorFunc(B, A), BA_Result); });
	}

	template <typename TargetType>
	void DescribeAllOperatorsForType(
		TargetType A,
		TargetType B,
		TOperatorType<TargetType> LessOperator,
		TOperatorType<TargetType> GreaterOperator,
		TOperatorType<TargetType> LessEqualOperator,
		TOperatorType<TargetType> GreaterEqualOperator)
	{
		Describe("<", [=, this]() { DescribeSingleOperatorTestCases(A, B, LessOperator, true, false, false); });
		Describe(">", [=, this]() { DescribeSingleOperatorTestCases(A, B, GreaterOperator, false, false, true); });
		Describe("<=", [=, this]() { DescribeSingleOperatorTestCases(A, B, LessEqualOperator, true, true, false); });
		Describe(">=", [=, this]() { DescribeSingleOperatorTestCases(A, B, GreaterEqualOperator, false, true, true); });
	}
END_DEFINE_SPEC(FOUULexicalOperatorLibrarySpec)

void FOUULexicalOperatorLibrarySpec::Define()
{
	TArray<FString> TestStrings;
	TestStrings.Add("Alpha");
	TestStrings.Add("Beta");
	TestStrings.Add("Beta2");
	TestStrings.Add("Gamma");

	for (int32 i = 0; i < TestStrings.Num() - 2; i++)
	{
		Describe("String", [=, this]() {
			const FString AString = TestStrings[i];
			const FString BString = TestStrings[i + 1];
			DescribeAllOperatorsForType<FString>(
				AString,
				BString,
				&UOUULexicalOperatorLibrary::Less_StringString,
				&UOUULexicalOperatorLibrary::Greater_StringString,
				&UOUULexicalOperatorLibrary::LessEqual_StringString,
				&UOUULexicalOperatorLibrary::GreaterEqual_StringString);
		});
		Describe("Name", [=, this]() {
			const FName AName = *TestStrings[i];
			const FName BName = *TestStrings[i + 1];
			DescribeAllOperatorsForType<FName>(
				AName,
				BName,
				&UOUULexicalOperatorLibrary::Less_NameName,
				&UOUULexicalOperatorLibrary::Greater_NameName,
				&UOUULexicalOperatorLibrary::LessEqual_NameName,
				&UOUULexicalOperatorLibrary::GreaterEqual_NameName);
		});
	}
}

#endif
