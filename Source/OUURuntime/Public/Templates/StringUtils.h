// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "InterfaceUtils.h"
#include "Templates/IsArithmetic.h"
#include "Traits/IsStringType.h"
#include "Traits/StringConversionTraits.h"

//----------------------------------------------------------------------------------------------------------------------

/** LexToString overload for UObject pointers */
FORCEINLINE FString LexToString(const UObject* O)
{
	return IsValid(O) ? O->GetName() : "None";
}

/** LexToString overload for TScriptInterface pointers */
template <typename T>
FORCEINLINE FString LexToString(TScriptInterface<T> Interface)
{
	return IsValidInterface(Interface) ? Interface.GetObject()->GetName() : "None";
}

/** Concept for a class that supports string conversion via ToString() member */
struct CMemberToStringConvertable
{
	template <typename ElementType>
	auto Requires(const ElementType& Val) -> decltype(Val.ToString());
};

/** LexToString overload for pointers to objects that are themselves string convertable with LexToString() */
template <typename T>
typename TEnableIf<
	TPointerIsConvertibleFromTo<T, UObject>::Value == false && TIsCharType<T>::Value == false
		&& TModels<CMemberToStringConvertable, T>::Value == false,
	FString>::Type
	LexToString(const T* Object)
{
	return (Object != nullptr) ? LexToString(*Object) : TEXT("nullptr");
}

/** LexToString overload for pointers to objects that have a ToString member - copy for const T* */
template <typename T>
typename TEnableIf<
	TPointerIsConvertibleFromTo<T, UObject>::Value == false && TIsArithmetic<T>::Value == false
		&& TIsCharType<T>::Value == false && TModels<CMemberToStringConvertable, T>::Value == true,
	FString>::Type
	LexToString(const T* Object)
{
	return (Object != nullptr) ? Object->ToString() : TEXT("nullptr");
}

/** LexToString overload for references to objects that have a ToString member - copy for const T& */
template <typename T>
typename TEnableIf<
	TIsArithmetic<T>::Value == false && TIsCharType<T>::Value == false
		&& TModels<CMemberToStringConvertable, T>::Value == true,
	FString>::Type
	LexToString(const T& Object)
{
	return Object.ToString();
}

/** LexToString overload for shared pointers */
template <typename T>
FString LexToString(TSharedPtr<T> Object)
{
	return Object.IsValid() ? LexToString(Object.Get()) : TEXT("nullptr");
}

/**
 * Concept for a class that supports string conversion via LexToString() 
 * Declared here, because clang requires the LexToString() template overloads to be defined before the concept template.
 */
struct CLexToStringConvertible
{
	template <typename ElementType>
	auto Requires(const ElementType& Val) -> decltype(LexToString(Val));
};

//----------------------------------------------------------------------------------------------------------------------

template <typename T>
FString LexToString_QuotedIfString(const T& Value)
{
	if (TIsStringType<T>::Value)
	{
		return FString::Printf(TEXT("\"%s\""), *LexToString(Value));
	}
	else
	{
		return LexToString(Value);
	}
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Interprets all elements of an array as LexToString-convertible objects and joins them
 * in a comma separated list enclosed by square brackets (similar to json arrays).
 * Any string types will be quoted.
 */
template <typename ElementType, typename AllocatorType>
FString ArrayToString(const TArray<ElementType, AllocatorType>& Array, const TCHAR* Separator = TEXT(", "))
{
	static_assert(
		TModels<CLexToStringConvertible, ElementType>::Value,
		"ElementType must be string convertible with LexToString()!");
	return FString::Printf(TEXT("[%s]"), *FString::JoinBy(Array, Separator, [](auto& Element) {
							   return LexToString_QuotedIfString<ElementType>(Element);
						   }));
}

//----------------------------------------------------------------------------------------------------------------------

/**
 * Interprets all elements of a map as LexToString-convertible objects and joins them
 * in a comma separated list enclosed by curly brackets (similar to json maps).
 * Any string types will be quoted.
 */
template <typename KeyType, typename ValueType, typename AllocatorType>
FString MapToString(const TMap<KeyType, ValueType, AllocatorType>& Map, const TCHAR* Separator = TEXT(", "))
{
	static_assert(
		TModels<CLexToStringConvertible, KeyType>::Value,
		"KeyType must be string convertible with LexToString()!");
	static_assert(
		TModels<CLexToStringConvertible, ValueType>::Value,
		"ValueType must be string convertible with LexToString()!");

	return FString::Printf(TEXT("{%s}"), *FString::JoinBy(Map, Separator, [](auto& Element) {
							   return FString::Printf(
								   TEXT("%s : %s"),
								   *LexToString_QuotedIfString<KeyType>(Element.Key),
								   *LexToString_QuotedIfString<ValueType>(Element.Value));
						   }));
}
