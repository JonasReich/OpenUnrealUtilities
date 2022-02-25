// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "InterfaceUtils.h"
#include "Traits/IsStringType.h"
#include "Traits/StringConversionTraits.h"

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

/** LexToString overload for pointers to objects that are themselves string convertable with LexToString() */
template <
	typename T,
	typename = typename TEnableIf<
		TIsPointer<T>::Value == true
		&& TPointerIsConvertibleFromTo<typename TRemovePointer<T>::Type, const UObject>::Value == false
		&& TModels<CMemberToStringConvertable, typename TRemovePointer<T>::Type>::Value == false>::Type>
FString LexToString(T Object)
{
	return (Object != nullptr) ? LexToString(*Object) : TEXT("nullptr");
}

/** LexToString overload for pointers to objects that have a ToString member */
template <
	typename T,
	typename = typename TEnableIf<
		TIsPointer<T>::Value == true
		&& TPointerIsConvertibleFromTo<typename TRemovePointer<T>::Type, const UObject>::Value == false
		&& TModels<CMemberToStringConvertable, typename TRemovePointer<T>::Type>::Value == true>::Type>
FString LexToString(T Object, int32 iOverloadArg = 0)
{
	return (Object != nullptr) ? Object->ToString() : TEXT("nullptr");
}

/** LexToString overload for references to objects that have a ToString member */
template <
	typename T,
	typename = typename TEnableIf<
		TIsPointer<T>::Value == false && TIsArithmetic<T>::Value == false
		&& TModels<CMemberToStringConvertable, T>::Value>::Type>
FString LexToString(const T& Object)
{
	return Object.ToString();
}

/** LexToString overload for shared pointers */
template <typename T>
FString LexToString(TSharedPtr<T> Object)
{
	return Object.IsValid() ? LexToString(Object.Get()) : TEXT("nullptr");
}

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
