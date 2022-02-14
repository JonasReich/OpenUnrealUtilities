// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Traits/IsInteger.h"

/**
 * Returns the exponentiation of an integer.
 *
 * Warning: Use with caution!
 *
 * This function does not have any overflow checks when used with runtime values,
 * because they would prohibit marking the function as contexpr.
 *
 * At compile time we cannot add any checks either, because they depend on the
 * intermediate steps of the calculation, which means they are not qualified for
 * static asserts.
 */
template <typename T>
constexpr T IntPow(T Base, T Power)
{
	static_assert(TIsInteger<T>::Value, "T must be an integer type");

	T Result = static_cast<T>(1);

	// additional termination condition for unsigned integers
	while (Power >= 0)
	{
		if (Power == 0)
			break;
		Result *= Base;
		Power -= 1;
	}
	return Result;
}

/**
 * Convert the decimal number to a pseudo binary number.
 * The digits of the input in base 2 match with the digits of the output in base 10.
 *
 * Example:
 * |        | Base 10     | Base 2                               |
 * |--------|-------------|--------------------------------------|
 * | Input  | 1024        | 0b10000000000                        |
 * | Output | 10000000000 | 0b1001010100000010111110010000000000 |
 */
template <typename T>
constexpr int64 ConvertDecimalToFakeBinary(T Decimal)
{
	static_assert(TIsInteger<T>::Value, "T must be an integer type");

	int64 Result = 0;
	int64 Digit = 0;
	while (Decimal)
	{
		const T Mask = static_cast<T>(1) << Digit;
		if (Decimal & Mask)
		{
			Result += IntPow<int64>(10, Digit);
		}
		Decimal &= ~(Mask);
		Digit += 1;
	}
	return Result;
}

/**
 * Count the digit of an integer in any base.
 * Default: Base 10 = decimal.
 * @returns the number of digits, e.g. 4 for input value 1024 in base 10
 */
template <typename T>
constexpr T NumDigits(typename TIdentity<T>::Type Value, T Base = 10)
{
	static_assert(TIsInteger<T>::Value, "T must be an integer type");

	T Digits = 0;
	if (Value < 0)
	{
		Value *= -1;
	}
	while (Value >= IntPow<T>(Base, Digits))
	{
		Digits += 1;
	}
	return Digits > 0 ? Digits : 1;
}

/**
 * Helper function for traits to get if a value is negative without raising compiler warnings.
 * Only signed integer types are checked against 0.
 */
template <typename T>
constexpr auto IsNegativeInteger(T Number) -> typename TEnableIf<TIsSigned<T>::Value, bool>::Type
{
	static_assert(TIsInteger<T>::Value, "T must be an integer type");
	return Number < 0;
}

/**
 * Helper function for traits to get if a value is negative without raising compiler warnings.
 * Only signed integer types are checked against 0.
 */
template <typename T>
constexpr auto IsNegativeInteger(T Number) -> typename TEnableIf<TIsSigned<T>::Value == false, bool>::Type
{
	static_assert(TIsInteger<T>::Value, "T must be an integer type");
	return false;
}

/**
 * Get the number of bits required to display a given number.
 */
template <typename T>
constexpr T GetMinBitSize(T Value)
{
	static_assert(TIsInteger<T>::Value, "T must be an integer type");

	const bool bSigned = IsNegativeInteger(Value);
	T N = 0;
	T ValueAsAbsInt = static_cast<T>(bSigned ? Value * -1 : Value);
	// continue unsetting bits until ValueAsAbsInt is 0
	while (ValueAsAbsInt)
	{
		// unset Nth bit
		ValueAsAbsInt &= ~(1 << N);
		N += 1;
	}

	if (bSigned)
	{
		N += 1;
	}

	return N > 0 ? N : 1;
}
