// Copyright (c) 2023 Jonas Reich & Contributors

#include "Traits/SizedIntegers.h"

// Static tests for min value integers
// This indirectly tests bit sized integers and min bit sized integers as well.

static_assert(std::is_same_v<TMinValueInteger<-1>::Type, int8>, "min int8 required for displaying -1!");
static_assert(std::is_same_v<TMinValueInteger<0>::Type, bool>, "min bool required for displaying +0!");
static_assert(std::is_same_v<TMinValueInteger<1>::Type, bool>, "min bool required for displaying +1!");
static_assert(std::is_same_v<TMinValueInteger<255>::Type, uint8>, "min uint8 required for displaying 255!");
static_assert(std::is_same_v<TMinValueInteger<256>::Type, uint16>, "min uint16 required for displaying 256!");
static_assert(std::is_same_v<TMinValueInteger<-255>::Type, int16>, "min int16 required for displaying -255!");
