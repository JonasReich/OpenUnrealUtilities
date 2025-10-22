// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "Misc/AssertionMacros.h"

struct FRecursionGuard : TScopeCounter<uint16>
{
public:
	FRecursionGuard(uint16& Counter, const uint16& RecursionLimit) :
		TScopeCounter(Counter), LimitReached(Counter > RecursionLimit)
	{
	}

	// Do NOT allow bool operator! otherwise it's easy to mis-use by declaring this as rvalue only.
	// We need this to actually be a scope guard for the entire function.
	// operator bool() const { return LimitReached; }

	bool WasLimitReached() const { return LimitReached; }

private:
	const bool LimitReached;
};

/**
 * Call the succeeding block of code if the enclosing function scope is called recursively more than RecursionLimit
 * times. Generally best placed at the start of a function for best legibility.
 *
 * Example:
 *
 * void Foo()
 * {
 *     ON_RECURSION_LIMIT_REACHED(50)
 *     {
 *         ensureMsgf(false, TEXT("Foo called recursively more than 50 times"));
 *         return;
 *     }
 *     Foo();
 * }
 */
#define ON_RECURSION_LIMIT_REACHED(RecursionLimit)                                                                     \
	static_assert(RecursionLimit > 1, "RecursionLimit must be a positive uint16");                                     \
	PRIVATE_ON_RECURSION_LIMIT_REACHED_IMPL(                                                                           \
		RecursionLimit,                                                                                                \
		PREPROCESSOR_JOIN(RecursionCounter, __LINE__),                                                                 \
		PREPROCESSOR_JOIN(RecursionGuard, __LINE__))

#define PRIVATE_ON_RECURSION_LIMIT_REACHED_IMPL(RecursionLimit, RecursionCounter, RecursionGuard)                      \
	static uint16 RecursionCounter = 0;                                                                                \
	const FRecursionGuard RecursionGuard{RecursionCounter, RecursionLimit};                                            \
	if (RecursionGuard.WasLimitReached())
