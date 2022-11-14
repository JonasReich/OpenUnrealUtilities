// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

// CMemberToStringConvertable and CLexToStringConvertible were moved to Templates/StringUtils.h
// Because clang requires the LexToString() template overloads to be defined before the conecept template.

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
