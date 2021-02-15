// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/** Concept for a class that supports string conversion via LexToString() */
struct CLexToStringConvertible
{
	template<typename ElementType>
	auto Requires(ElementType It) -> decltype(LexToString(DeclVal<ElementType>()));
};

/** Concept for a class that supports string conversion via ToString() member */
struct CMemberToStringConvertable
{
	template<typename ElementType>
	auto Requires(ElementType It) -> decltype(DeclVal<ElementType>().ToString());
};

/** Concept for a class that supports parsing from FString/TCHAR* via LexTryParseString() */
struct CLexTryParseString_Parseable
{
	template <typename T>
	auto Requires(T& Val) -> decltype(LexTryParseString(Val, *DeclVal<FString>()));
};

/** Concept for a class that supports parsing from FString/TCHAR* via LexFromString() */
struct CLexFromString_Parseable
{
	template <typename T>
	auto Requires(T& Val) -> decltype(LexFromString(Val, *DeclVal<FString>()));
};
