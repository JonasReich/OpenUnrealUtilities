// Copyright (c) 2022 Jonas Reich

#include "Traits/SizedIntegers.h"

// Static tests for min value integers
// This indirectly tests bit sized integers and min bit sized integers as well.

static_assert(TIsSame<TMinValueInteger<-1>::Type, int8>::Value, "min int8 required for displaying -1!");
static_assert(TIsSame<TMinValueInteger<0>::Type, bool>::Value, "min bool required for displaying +0!");
static_assert(TIsSame<TMinValueInteger<1>::Type, bool>::Value, "min bool required for displaying +1!");
static_assert(TIsSame<TMinValueInteger<255>::Type, uint8>::Value, "min uint8 required for displaying 255!");
static_assert(TIsSame<TMinValueInteger<256>::Type, uint16>::Value, "min uint16 required for displaying 256!");
static_assert(TIsSame<TMinValueInteger<-255>::Type, int16>::Value, "min int16 required for displaying -255!");
