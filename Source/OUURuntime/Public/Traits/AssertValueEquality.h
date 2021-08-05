// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/**
 * Use this trait if you want to compare values in a different trait/asserts and want to see what the values are if compilation fails.
 * Most compilers (like MSVC) only show the evaluated values if they were part of template parameters.
 */
template<typename T, T A, T B>
struct TAssertValuesEqual
{
	static const bool Value = A == B; 
	static_assert(Value, "A and B are not equal");
};

/**
 * Use this trait if you want to compare values in a different trait/asserts and want to see what the values are if compilation fails.
 * Most compilers (like MSVC) only show the evaluated values if they were part of template parameters.
 */
template<typename T, T A, T B>
struct TAssertValuesUnequal
{
	static const bool Value = A != B;
	static_assert(Value, "A and B are equal");
};
