// Copyright (c) 2021 Jonas Reich

#pragma once

/**
* Traits class which tests if a type is one of unreal's string types
*/
template <typename T>
struct TIsStringType
{
	// Ensure that implicitly convertible types to FString (e.g. FString, TCHAR*) are automatically treated as string
	// and we only have to add a minimum set of type overloads
	enum { Value = TIsConstructible<FString, T>::Value };
};

template <> struct TIsStringType<FName> { enum { Value = true }; };
template <> struct TIsStringType<FText> { enum { Value = true }; };

template <typename T> struct TIsStringType<const          T> { enum { Value = TIsStringType<T>::Value }; };
template <typename T> struct TIsStringType<      volatile T> { enum { Value = TIsStringType<T>::Value }; };
template <typename T> struct TIsStringType<const volatile T> { enum { Value = TIsStringType<T>::Value }; };
