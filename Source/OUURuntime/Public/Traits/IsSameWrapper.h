// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include <type_traits>

/**
 * Workaround that allows us to is std::is_same_v in combination with TOr and TAnd, because those expect ::Value to be
 * upper case.
 */
template <typename A, typename B>
struct TIsSameWrapper
{
	static constexpr bool Value = std::is_same_v<A, B>;
};