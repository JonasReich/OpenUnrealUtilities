// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

/** Get the underlying integer type of an enum or enum class. */
template <typename T>
using TUnderlyingType = __underlying_type(T);
