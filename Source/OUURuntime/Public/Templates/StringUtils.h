// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Traits/IsStringType.h"
#include "Traits/StringConversionTraits.h"

/** LexToString overload for UObject pointers */
FORCEINLINE FString LexToString(const UObject* O)
{
	return IsValid(O) ? O->GetName() : "Invalid";
}

/** LexToString overload for pointers to objects that are themselves string convertable with LexToString() */
template <typename T,
          typename = typename TEnableIf<
	          TIsPointer<T>::Value == true &&
	          TPointerIsConvertibleFromTo<typename TRemovePointer<T>::Type, const UObject>::Value == false &&
	          TModels<CMemberToStringConvertable, typename TRemovePointer<T>::Type>::Value == false
          >::Type>
FString LexToString(T Object)
{
	return (Object != nullptr) ? LexToString(*Object) : TEXT("nullptr");
}

/** LexToString overload for pointers to objects that have a ToString member */
template <typename T,
          typename = typename TEnableIf<
	          TIsPointer<T>::Value == true &&
	          TPointerIsConvertibleFromTo<typename TRemovePointer<T>::Type, const UObject>::Value == false &&
	          TModels<CMemberToStringConvertable, typename TRemovePointer<T>::Type>::Value == true
          >::Type>
FString LexToString(T Object, int32 iOverloadArg = 0)
{
	return (Object != nullptr) ? Object->ToString() : TEXT("nullptr");
}

/** LexToString overload for references to objects that have a ToString member */
template <typename T,
          typename = typename TEnableIf<
	          TIsPointer<T>::Value == false &&
	          TModels<CMemberToStringConvertable, T>::Value
          >::Type>
FString LexToString(const T& Object)
{
	return Object.ToString();
}

/**
 * Interprets all elements of an array as LexToString-convertible objects and joins them
 * in a comma separated list enclosed by square brackets.
 * Any string types will be quoted. 
 */
template <typename ElementType, typename AllocatorType>
FString ArrayToString(const TArray<ElementType, AllocatorType>& Array)
{
	static_assert(TModels<CLexToStringConvertible, ElementType>::Value, "T must be string convertible with LexToString()!");
	return FString::Printf(TEXT("[%s]"), *FString::JoinBy(Array, TEXT(", "), [](auto& Element)
	{
		if (TIsStringType<ElementType>::Value)
		{
			return FString::Printf(TEXT("\"%s\""), *LexToString(Element));
		}
		else
		{
			return LexToString(Element);
		}
	}));
}
