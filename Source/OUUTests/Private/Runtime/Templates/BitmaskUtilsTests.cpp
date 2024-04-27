// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Templates/BitmaskUtils.h"

	#define OUU_TEST_CATEGORY OpenUnrealUtilities.Runtime.Templates
	#define OUU_TEST_TYPE	  BitmaskUtils

//////////////////////////////////////////////////////////////////////////
// Utility types for testing
//////////////////////////////////////////////////////////////////////////

// Test enum that is declared like a BlueprintType UENUM minus the UHT metadata
enum class ENaturalTestEnum : uint8
{
	Zero,
	One,
	Two,
	Three,
	Four
};

DECLARE_ENUM_SEQUENCE(ENaturalTestEnum, EEnumSequenceType::Natural);

// Shared bitmask used for all TestAllBits_* and TestAnyBits_* tests
constexpr int32 TestMultipleNaturalBits_Bitmask = 7;

enum class EPow2TestEnum : uint8
{
	FirstBit = 0b00001,
	SecondBit = 0b00010,
	ThirdBit = 0b00100,
	FourthBit = 0b01000,
	FifthBit = 0b10000
};

DECLARE_BITMASK_OPERATORS(EPow2TestEnum);

// Shared bitmask used for all TestAllBits_* and TestAnyBits_* tests
constexpr int32 TestMultiplePow2Bits_Bitmask = 7;

//////////////////////////////////////////////////////////////////////////
// Statically test bitmask operators
//////////////////////////////////////////////////////////////////////////

constexpr EPow2TestEnum CombinedCases = EPow2TestEnum::FirstBit | EPow2TestEnum::SecondBit;
static_assert(
	static_cast<int32>(CombinedCases) == 0b11,
	"combining two enum values results in the correct numerical result");
// #TODO-OUU I did not find a clang compatible way to implement implicit bool conversion
// static_assert(EPow2TestEnum::FirstBit, "a single bitmask value can be implicitly converted to bool");
static_assert(EPow2TestEnum::FirstBit | EPow2TestEnum::SecondBit, "combining bits can be converted implicitly to bool");
static_assert(!(EPow2TestEnum::FirstBit & EPow2TestEnum::SecondBit), "yields correct result -> 0 = false");

//////////////////////////////////////////////////////////////////////////
// EnumValueAsBitmask
//////////////////////////////////////////////////////////////////////////

/**
 * 0: natural/pow2?
 * 1: enum input
 * 2: bitmask result
 */
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(EnumValueAsBitmask, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("true|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|1|2")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|2|4")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|3|8")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|4|16")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|2|2")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|4|4")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|8|8")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|16|16")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(EnumValueAsBitmask)
{
	// Arrange
	const FAutomationTestParameterParser Parser{Parameters};
	const bool bNaturalEnum = Parser.GetValue<bool>(0);
	const int32 ExpectedBitmask = Parser.GetValue<int32>(2);

	// Act
	const int32 Bitmask = bNaturalEnum
		? OUU::Runtime::BitmaskUtils::EnumValueAsBitmask(Parser.GetValue<ENaturalTestEnum>(1))
		: OUU::Runtime::BitmaskUtils::EnumValueAsBitmask(Parser.GetValue<EPow2TestEnum>(1));

	// Assert
	TestEqual("Bitmask", Bitmask, ExpectedBitmask);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// EnumValuesAsBitmask
//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(EnumValuesAsBitmask, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	constexpr int32 ExpectedBitmask = 3;

	// Act
	constexpr int32 NaturalBitmask =
		OUU::Runtime::BitmaskUtils::EnumValuesAsBitmask({ENaturalTestEnum::Zero, ENaturalTestEnum::One});
	constexpr int32 Pow2Bitmask =
		OUU::Runtime::BitmaskUtils::EnumValuesAsBitmask({EPow2TestEnum::FirstBit, EPow2TestEnum::SecondBit});

	// Assert
	TestEqual("Bitmask (Natural)", NaturalBitmask, ExpectedBitmask);
	TestEqual("Bitmask (Pow2)", Pow2Bitmask, ExpectedBitmask);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// SetBit(s)
//////////////////////////////////////////////////////////////////////////

/**
 * 0: natural/pow2?
 * 1: Original Bitmask
 * 2: Enum to set
 * 3: Expected Result (Bitmask)
 */
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(SetBit, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("true|0|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|1|0|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|1|1|3")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|1|2|5")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|0|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|1|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|1|2|3")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|2|1|3")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|3|1|3")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|3|2|3")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|4|1|5")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(SetBit)
{
	// Arrange
	const FAutomationTestParameterParser Parser{Parameters};
	const bool bNaturalEnum = Parser.GetValue<bool>(0);
	int32 Bitmask = Parser.GetValue<int32>(1);
	const int32 ExpectedResult = Parser.GetValue<int32>(3);

	// Act
	if (bNaturalEnum)
	{
		OUU::Runtime::BitmaskUtils::SetBit(Bitmask, Parser.GetValue<ENaturalTestEnum>(2));
	}
	else
	{
		OUU::Runtime::BitmaskUtils::SetBit(Bitmask, Parser.GetValue<EPow2TestEnum>(2));
	}

	// Assert
	TestEqual("Bitmask", Bitmask, ExpectedResult);
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(SetBits, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	constexpr int32 ExpectedBitmask = 7;
	int32 NaturalBitmask = 4;
	int32 Pow2Bitmask = 4;

	// Act
	OUU::Runtime::BitmaskUtils::SetBits(NaturalBitmask, {ENaturalTestEnum::Zero, ENaturalTestEnum::One});
	OUU::Runtime::BitmaskUtils::SetBits(Pow2Bitmask, {EPow2TestEnum::FirstBit, EPow2TestEnum::SecondBit});

	// Assert
	TestEqual("Bitmask (Natural)", NaturalBitmask, ExpectedBitmask);
	TestEqual("Bitmask (Pow2)", NaturalBitmask, ExpectedBitmask);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// ClearBit(s)
//////////////////////////////////////////////////////////////////////////

/**
 * 0: natural/pow2?
 * 1: Original Bitmask
 * 2: Enum to clear
 * 3: Expected Result (Bitmask)
 */
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(ClearBit, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("true|0|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|1|0|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|3|0|2")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|3|1|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|0|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|1|1|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|2|1|2")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|2|2|0")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|3|1|2")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|3|2|1")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(ClearBit)
{
	// Arrange
	const FAutomationTestParameterParser Parser{Parameters};
	const bool bNaturalEnum = Parser.GetValue<bool>(0);
	int32 Bitmask = Parser.GetValue<int32>(1);
	const int32 ExpectedResult = Parser.GetValue<int32>(3);

	// Act
	if (bNaturalEnum)
	{
		OUU::Runtime::BitmaskUtils::ClearBit(Bitmask, Parser.GetValue<ENaturalTestEnum>(2));
	}
	else
	{
		OUU::Runtime::BitmaskUtils::ClearBit(Bitmask, Parser.GetValue<EPow2TestEnum>(2));
	}

	// Assert
	TestEqual("Bitmask", Bitmask, ExpectedResult);
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(ClearBits, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	constexpr int32 ExpectedBitmask = 4;
	int32 NaturalBitmask = 7;
	int32 Pow2Bitmask = 7;

	// Act
	OUU::Runtime::BitmaskUtils::ClearBits(NaturalBitmask, {ENaturalTestEnum::Zero, ENaturalTestEnum::One});
	OUU::Runtime::BitmaskUtils::ClearBits(Pow2Bitmask, {EPow2TestEnum::FirstBit, EPow2TestEnum::SecondBit});

	// Assert
	TestEqual("Bitmask (Natural)", NaturalBitmask, ExpectedBitmask);
	TestEqual("Bitmask (Pow2)", Pow2Bitmask, ExpectedBitmask);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// TestBit
//////////////////////////////////////////////////////////////////////////

/**
 * 0: natural/pow2?
 * 1: Bitmask
 * 2: Enum to test
 * 3: Expected Result (boolean)
 */
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(TestBit, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("true|0|0|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|0|1|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|1|0|true")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|1|1|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|3|0|true")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|3|1|true")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|3|2|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("true|3|3|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|0|1|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|0|2|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|1|1|true")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|1|2|false")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|3|1|true")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|3|2|true")
OUU_COMPLEX_AUTOMATION_TESTCASE("false|3|4|false")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(TestBit)
{
	// Arrange
	const FAutomationTestParameterParser Parser{Parameters};
	const bool bNaturalEnum = Parser.GetValue<bool>(0);
	const int32 Bitmask = Parser.GetValue<int32>(1);
	const bool ExpectedResult = Parser.GetValue<bool>(3);

	// Act
	const bool ActualResult = bNaturalEnum
		? OUU::Runtime::BitmaskUtils::TestBit(Bitmask, Parser.GetValue<ENaturalTestEnum>(2))
		: OUU::Runtime::BitmaskUtils::TestBit(Bitmask, Parser.GetValue<EPow2TestEnum>(2));

	// Assert
	TestEqual("TestBit result", ActualResult, ExpectedResult);
	return true;
}

//////////////////////////////////////////////////////////////////////////
// TestAllBits
//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestAllBits_Complete, DEFAULT_OUU_TEST_FLAGS)
{
	TestTrue(
		"TestAllBits for matching set of flags (Natural)",
		OUU::Runtime::BitmaskUtils::TestAllBits(
			TestMultipleNaturalBits_Bitmask,
			{ENaturalTestEnum::Zero, ENaturalTestEnum::One, ENaturalTestEnum::Two}));
	TestTrue(
		"TestAllBits for matching set of flags (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAllBits(
			TestMultiplePow2Bits_Bitmask,
			{EPow2TestEnum::FirstBit, EPow2TestEnum::SecondBit, EPow2TestEnum::ThirdBit}));
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestAllBits_Incomplete, DEFAULT_OUU_TEST_FLAGS)
{
	TestTrue(
		"TestAllBits for incomplete set of flags (Natural)",
		OUU::Runtime::BitmaskUtils::TestAllBits(
			TestMultipleNaturalBits_Bitmask,
			{ENaturalTestEnum::Zero, ENaturalTestEnum::Two}));
	TestTrue(
		"TestAllBits for incomplete set of flags (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAllBits(
			TestMultiplePow2Bits_Bitmask,
			{EPow2TestEnum::FirstBit, EPow2TestEnum::ThirdBit}));
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestAllBits_TooMany, DEFAULT_OUU_TEST_FLAGS)
{
	TestFalse(
		"TestAllBits for too many flags (Natural)",
		OUU::Runtime::BitmaskUtils::TestAllBits(
			TestMultipleNaturalBits_Bitmask,
			{ENaturalTestEnum::Zero, ENaturalTestEnum::One, ENaturalTestEnum::Two, ENaturalTestEnum::Three}));
	TestFalse(
		"TestAllBits for too many flags (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAllBits(
			TestMultiplePow2Bits_Bitmask,
			{EPow2TestEnum::FirstBit, EPow2TestEnum::SecondBit, EPow2TestEnum::ThirdBit, EPow2TestEnum::FourthBit}));
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TestAllBits_AllWrong, DEFAULT_OUU_TEST_FLAGS)
{
	TestFalse(
		"TestAllBits for completely wrong flags (Natural)",
		OUU::Runtime::BitmaskUtils::TestAllBits(TestMultipleNaturalBits_Bitmask, {ENaturalTestEnum::Three}));
	TestFalse(
		"TestAllBits for completely wrong flags (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAllBits(TestMultiplePow2Bits_Bitmask, {EPow2TestEnum::FourthBit}));

	return true;
}

//////////////////////////////////////////////////////////////////////////
// TestAnyBits
//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(Natural_TestAnyBits_Complete, DEFAULT_OUU_TEST_FLAGS)
{
	TestTrue(
		"TestAnyBits for matching set of flags (Natural)",
		OUU::Runtime::BitmaskUtils::TestAnyBits(
			TestMultipleNaturalBits_Bitmask,
			{ENaturalTestEnum::Zero, ENaturalTestEnum::One, ENaturalTestEnum::Two}));
	TestTrue(
		"TestAnyBits for matching set of flags must be true (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAnyBits(
			TestMultiplePow2Bits_Bitmask,
			{EPow2TestEnum::FirstBit, EPow2TestEnum::SecondBit, EPow2TestEnum::ThirdBit}));
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(Natural_TestAnyBits_Incomplete, DEFAULT_OUU_TEST_FLAGS)
{
	TestTrue(
		"TestAnyBits for incomplete set of flags (Natural)",
		OUU::Runtime::BitmaskUtils::TestAnyBits(
			TestMultipleNaturalBits_Bitmask,
			{ENaturalTestEnum::Zero, ENaturalTestEnum::Two}));
	TestTrue(
		"TestAnyBits for incomplete set of flags (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAnyBits(
			TestMultiplePow2Bits_Bitmask,
			{EPow2TestEnum::FirstBit, EPow2TestEnum::ThirdBit}));
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(Natural_TestAnyBits_TooMany, DEFAULT_OUU_TEST_FLAGS)
{
	TestTrue(
		"TestAnyBits for too many flags (Natural)",
		OUU::Runtime::BitmaskUtils::TestAnyBits(
			TestMultipleNaturalBits_Bitmask,
			{ENaturalTestEnum::Zero, ENaturalTestEnum::One, ENaturalTestEnum::Two, ENaturalTestEnum::Three}));
	TestTrue(
		"TestAnyBits for too many flags (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAnyBits(
			TestMultiplePow2Bits_Bitmask,
			{EPow2TestEnum::FirstBit, EPow2TestEnum::SecondBit, EPow2TestEnum::ThirdBit, EPow2TestEnum::FourthBit}));
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(Natural_TestAnyBits_AllWrong, DEFAULT_OUU_TEST_FLAGS)
{
	TestFalse(
		"TestAnyBits for completely wrong flags",
		OUU::Runtime::BitmaskUtils::TestAnyBits(TestMultipleNaturalBits_Bitmask, {ENaturalTestEnum::Three}));
	TestFalse(
		"TestAnyBits for completely wrong flags (Pow2)",
		OUU::Runtime::BitmaskUtils::TestAnyBits(TestMultiplePow2Bits_Bitmask, {EPow2TestEnum::FourthBit}));
	return true;
}

//////////////////////////////////////////////////////////////////////////

	#undef OUU_TEST_CATEGORY
	#undef OUU_TEST_TYPE

#endif
