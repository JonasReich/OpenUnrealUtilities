// Copyright (c) 2022 Jonas Reich

#pragma once

/** Get the underlying integer type of an enum or enum class. */
template <typename T>
using TUnderlyingType = __underlying_type(T);
