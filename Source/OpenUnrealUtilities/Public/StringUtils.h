// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/** Concept for a class that supports LexToString() */
struct CLexToStringConvertable
{
	template<typename ElementType>
	auto Requires(ElementType It) -> decltype(LexToString(DeclVal<ElementType>()));
};

struct CMemberToStringConvertable
{
	template<typename ElementType>
	auto Requires(ElementType It) -> decltype(DeclVal<ElementType>().ToString());
};

FORCEINLINE FString LexToString(const UObject* O)
{
	return IsValid(O) ? O->GetName() : "Invalid";
}

template<typename T, typename = typename TEnableIf<
	TIsPointer<T>::Value == true &&
	TPointerIsConvertibleFromTo<TRemovePointer<T>::Type, const UObject>::Value == false &&
	TModels<CMemberToStringConvertable, TRemovePointer<T>::Type>::Value == false
	>::Type>
FString LexToString(T Object)
{
	return (Object != nullptr) ? LexToString(*Object) : TEXT("nullptr");
}

// Overload for pointers to objects that are already LexToString convertable
template<typename T, typename = typename TEnableIf<
	TIsPointer<T>::Value == true &&
	TPointerIsConvertibleFromTo<TRemovePointer<T>::Type, const UObject>::Value == false &&
	TModels<CMemberToStringConvertable, TRemovePointer<T>::Type>::Value == true
	>::Type>
FString LexToString(T Object, int32 iOverloadArg = 0)
{
	return (Object != nullptr) ? Object->ToString() : TEXT("nullptr");
}

template<typename T, typename = typename TEnableIf<
	TIsPointer<T>::Value == false &&
	TModels<CMemberToStringConvertable, T>::Value
>::Type>
FString LexToString(const T& Object)
{
	return Object.ToString();
}
