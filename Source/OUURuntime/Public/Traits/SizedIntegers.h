// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Math/StaticIntegerMath.h"
#include "Traits/AssertValueEquality.h"

/** Trait to assert that the number of mutable bits in an integer type is as expected, */
template <typename T, SIZE_T NumBits>
struct TAssertBitSizeEquality
{
	static const SIZE_T NumBitsInT = std::is_same_v<T, bool> ? 1 : sizeof(T) * 8;
	static const bool Value = TAssertValuesEqual<SIZE_T, NumBitsInT, NumBits>::Value;
	static_assert(Value, "Number of bits in T is not equal to NumBits");
};

/**
 * Trait to retrieve a signed or unsigned integer with exactly BitSize mutable bits.
 * If the integer is singed, the sign bit must be factored into the bit size.
 */
template <SIZE_T BitSize, bool bSigned>
struct TBitSizedInteger
{
	using Type = void;
	static_assert(
		bSigned || TAssertValuesEqual<SIZE_T, BitSize, -1>::Value,
		"Size is not a valid unsigned integer size. See template specializations for TBitSizedInteger in "
		"IntegerSize.h");
	static_assert(
		!bSigned || TAssertValuesEqual<SIZE_T, BitSize, -1>::Value,
		"Size is not a valid signed integer size. See template specializations for TBitSizedInteger in IntegerSize.h");
};

// Define specializations for TBitSizedInteger
#define T_SIZED_INTEGER_IMPL(Size, bSigned, IntType)                                                                   \
	template <>                                                                                                        \
	struct TBitSizedInteger<Size, bSigned>                                                                             \
	{                                                                                                                  \
		using Type = IntType;                                                                                          \
	};                                                                                                                 \
	static_assert(TAssertBitSizeEquality<IntType, Size>::Value, "Sized integer implementation has wrong size!");

T_SIZED_INTEGER_IMPL(1, false, bool);
T_SIZED_INTEGER_IMPL(8, true, int8);
T_SIZED_INTEGER_IMPL(8, false, uint8);
T_SIZED_INTEGER_IMPL(16, true, int16);
T_SIZED_INTEGER_IMPL(16, false, uint16);
T_SIZED_INTEGER_IMPL(32, true, int32);
T_SIZED_INTEGER_IMPL(32, false, uint32);
T_SIZED_INTEGER_IMPL(64, true, int64);
T_SIZED_INTEGER_IMPL(64, false, uint64);

#undef T_SIZED_INTEGER_IMPL

/**
 * Trait that returns the smallest integer type that has at least MinBitCount mutable bits.
 * Can be signed or unsigned.
 * If signed, the sign bit is included in MinBitCount.
 */
template <SIZE_T MinBitCount, bool bSigned>
struct TMinBitSizedInteger
{
	static constexpr SIZE_T NearestIntegerBitCount(SIZE_T InBitCount)
	{
		if (InBitCount <= 1)
			return 1;
		if (InBitCount <= 8)
			return 8;
		if (InBitCount <= 16)
			return 16;
		if (InBitCount <= 32)
			return 32;
		if (InBitCount <= 64)
			return 64;
		return 0;
	}

	using Type = typename TBitSizedInteger<NearestIntegerBitCount(MinBitCount), bSigned>::Type;
};

/** Trait that returns the smallest integer type which has enough mutable bits for the target number */
template <int64 TargetNumber>
struct TMinValueInteger
{
	static const bool bSigned = IsNegativeInteger(TargetNumber);
	static const SIZE_T MinBits = GetMinBitSize(TargetNumber);
	using Type = typename TMinBitSizedInteger<MinBits, bSigned>::Type;
};
