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
 * Call the succeeding block of code if the enclosing scope is called recursively more than specified number of times.
 * Generally best placed at the start of a function for best legibility.
 * @param RecursionLimit: How often the scope may be re-entered recursively before triggering.
 */
#define ON_RECURSION_LIMIT_REACHED(RecursionLimit)                                                                     \
	PRIVATE_ON_RECURSION_LIMIT_REACHED_IMPL(                                                                           \
		RecursionLimit,                                                                                                \
		PREPROCESSOR_JOIN(RecursionCounter, __LINE__),                                                                 \
		PREPROCESSOR_JOIN(RecursionGuard, __LINE__))

/**
 * Easy to use version of an ensure and return if a recursion limit was reached.
 * @param RecursionLimit: How often the scope may be re-entered recursively before triggering the ensure.
 */
#define ENSURE_RECURSION_LIMIT_AND_RETURN(RecursionLimit, ...)                                                         \
	ON_RECURSION_LIMIT_REACHED(RecursionLimit)                                                                         \
	{                                                                                                                  \
		ensureMsgf(false, TEXT("Recursion limit (%i) reached"), RecursionLimit);                                       \
		return __VA_ARGS__;                                                                                            \
	}

#define PRIVATE_ON_RECURSION_LIMIT_REACHED_IMPL(RecursionLimit, RecursionCounter, RecursionGuard)                      \
	static thread_local uint16 RecursionCounter = 0;                                                                   \
	const FRecursionGuard RecursionGuard{RecursionCounter, RecursionLimit};                                            \
	if (RecursionGuard.WasLimitReached())
