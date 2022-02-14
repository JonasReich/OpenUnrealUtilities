// Copyright (c) 2021 Jonas Reich

#include "Math/StaticIntegerMath.h"

#include "Traits/AssertValueEquality.h"

// Static tests for IntPow()

template <SIZE_T Base, SIZE_T Power, SIZE_T ExpectedResult>
struct TAssertIntPowEquals
{
	static const SIZE_T TestResult = IntPow<SIZE_T>(Base, Power);
	static const bool Value = TAssertValuesEqual<SIZE_T, TestResult, ExpectedResult>::Value;
};

static_assert(TAssertIntPowEquals<10, 0, 1>::Value, "10^0 = 1");
static_assert(TAssertIntPowEquals<10, 1, 10>::Value, "10^1 = 10");
static_assert(TAssertIntPowEquals<10, 2, 100>::Value, "10^2 = 100");
static_assert(TAssertIntPowEquals<10, 3, 1000>::Value, "10^3 = 1000");
static_assert(TAssertIntPowEquals<2, 10, 1024>::Value, "2^10 = 1024");

// Static tests for ConvertDecimalToFakeBinary()

static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(0), 0>::Value, "0 as binary");
static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(1), 1>::Value, "1 as binary");
static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(2), 10>::Value, "2 as binary");
static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(3), 11>::Value, "3 as binary");
static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(4), 100>::Value, "4 as binary");
static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(5), 101>::Value, "5 as binary");
static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(6), 110>::Value, "6 as binary");
static_assert(TAssertValuesEqual<SIZE_T, ConvertDecimalToFakeBinary(7), 111>::Value, "7 as binary");

static_assert(TAssertValuesEqual<int64, ConvertDecimalToFakeBinary<int8>(-1), 11111111>::Value, "-1 as binary");
static_assert(TAssertValuesEqual<int64, ConvertDecimalToFakeBinary<int8>(-2), 11111110>::Value, "-2 as binary");
static_assert(TAssertValuesEqual<int64, ConvertDecimalToFakeBinary<int8>(-3), 11111101>::Value, "-3 as binary");

static_assert(
	TAssertValuesEqual<int64, ConvertDecimalToFakeBinary<int32>(1024), 10000000000>::Value,
	"1024 as fake-binary");
static_assert(
	TAssertValuesEqual<int64, ConvertDecimalToFakeBinary<int32>(0b10000000000), 10000000000>::Value,
	"1024 as fake-binary");
static_assert(
	TAssertValuesEqual<int64, ConvertDecimalToFakeBinary<int32>(1025), 10000000001>::Value,
	"1025 as fake-binary");
static_assert(
	TAssertValuesEqual<int64, ConvertDecimalToFakeBinary<int32>(0b10000000001), 10000000001>::Value,
	"1025 as fake-binary");

// Static tests for NumDigits

static_assert(TAssertValuesEqual<int32, NumDigits<int32>(-11), 2>::Value, "-11 has 2 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(-10), 2>::Value, "-10 has 2 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(-1), 1>::Value, "-1 has 1 digit");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(1), 1>::Value, "1 has 1 digit");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(2), 1>::Value, "2 has 1 digit");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(9), 1>::Value, "9 has 1 digit");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(10), 2>::Value, "10 has 2 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(11), 2>::Value, "11 has 2 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(100), 3>::Value, "100 has 3 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(110), 3>::Value, "110 has 3 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(111), 3>::Value, "111 has 3 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(999), 3>::Value, "999 has 3 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(1000), 4>::Value, "1000 has 4 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(1001), 4>::Value, "1001 has 4 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(1010), 4>::Value, "1010 has 4 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(1011), 4>::Value, "1011 has 4 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(1111), 4>::Value, "1111 has 4 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(2000), 4>::Value, "2000 has 4 digits");
static_assert(TAssertValuesEqual<int32, NumDigits<int32>(9999), 4>::Value, "9999 has 4 digits");

static_assert(TAssertValuesEqual<SIZE_T, NumDigits<int32>(0, 2), 1>::Value, "0 in base 2 has 1 digit");
static_assert(TAssertValuesEqual<SIZE_T, NumDigits<int32>(1, 2), 1>::Value, "1 in base 2 has 1 digit");
static_assert(TAssertValuesEqual<SIZE_T, NumDigits<int32>(2, 2), 2>::Value, "2 in base 2 has 2 digits");
static_assert(TAssertValuesEqual<SIZE_T, NumDigits<int32>(7, 2), 3>::Value, "7 in base 2 has 3 digits");
static_assert(TAssertValuesEqual<SIZE_T, NumDigits<int32>(1024, 2), 11>::Value, "1024 in base 2 has 11 digits");

// Static tests for GetMinBitSize

static_assert(
	TAssertValuesEqual<int32, GetMinBitSize(-3), 3>::Value,
	"GetMinSize<-3>() returns wrong value! (should be 3)");
static_assert(
	TAssertValuesEqual<int32, GetMinBitSize(+5), 3>::Value,
	"GetMinSize<+5>() returns wrong value! (should be 3)");
static_assert(
	TAssertValuesEqual<int32, GetMinBitSize(+0), 1>::Value,
	"GetMinSize<+0>() returns wrong value! (should be 1)");
static_assert(
	TAssertValuesEqual<int32, GetMinBitSize(-1), 2>::Value,
	"GetMinSize<-1>() returns wrong value! (should be 2)");
static_assert(
	TAssertValuesEqual<int32, GetMinBitSize(-5), 4>::Value,
	"GetMinSize<-5>() returns wrong value! (should be 4)");
static_assert(
	TAssertValuesEqual<int32, GetMinBitSize(+5), 3>::Value,
	"GetMinSize<+5>() returns wrong value! (should be 3)");
