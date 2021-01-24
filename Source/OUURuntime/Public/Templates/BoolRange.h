// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/**
 * Utility function to be used in ranged based for loops for adding bool variants to code.
 * Especially useful in automated testcase generation (e.g. test with feature toggled on and off).
 */
FORCEINLINE TArray<bool, TInlineAllocator<2>> BoolRange()
{
	return {false, true};
}
