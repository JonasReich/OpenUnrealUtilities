// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/**
 * Traits class which tests if a type is an integer type.
 */
template <typename T>
struct TIsInteger
{
	enum { Value = false };
};

template <> struct TIsInteger<uint8> { enum { Value = true }; };
template <> struct TIsInteger<uint16> { enum { Value = true }; };
template <> struct TIsInteger<uint32> { enum { Value = true }; };
template <> struct TIsInteger<uint64> { enum { Value = true }; };
template <> struct TIsInteger<int8> { enum { Value = true }; };
template <> struct TIsInteger<int16> { enum { Value = true }; };
template <> struct TIsInteger<int32> { enum { Value = true }; };
template <> struct TIsInteger<int64> { enum { Value = true }; };

template <typename T> struct TIsInteger<const          T> { enum { Value = TIsInteger<T>::Value }; };
template <typename T> struct TIsInteger<      volatile T> { enum { Value = TIsInteger<T>::Value }; };
template <typename T> struct TIsInteger<const volatile T> { enum { Value = TIsInteger<T>::Value }; };
