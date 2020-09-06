// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Traits/StringConversionTraits.h"

/** LexToString overload for UObject pointers */
FORCEINLINE FString LexToString(const UObject* O)
{
	return IsValid(O) ? O->GetName() : "Invalid";
}

/** LexToString overload for pointers to objects that are themselves string convertable with LexToString() */
template<typename T, typename = typename TEnableIf<
	TIsPointer<T>::Value == true &&
	TPointerIsConvertibleFromTo<TRemovePointer<T>::Type, const UObject>::Value == false &&
	TModels<CMemberToStringConvertable, TRemovePointer<T>::Type>::Value == false
	>::Type>
FString LexToString(T Object)
{
	return (Object != nullptr) ? LexToString(*Object) : TEXT("nullptr");
}

/** LexToString overload for pointers to objects that have a ToString member */
template<typename T, typename = typename TEnableIf<
	TIsPointer<T>::Value == true &&
	TPointerIsConvertibleFromTo<TRemovePointer<T>::Type, const UObject>::Value == false &&
	TModels<CMemberToStringConvertable, TRemovePointer<T>::Type>::Value == true
	>::Type>
FString LexToString(T Object, int32 iOverloadArg = 0)
{
	return (Object != nullptr) ? Object->ToString() : TEXT("nullptr");
}

/** LexToString overload for references to objects that have a ToString member */
template<typename T, typename = typename TEnableIf<
	TIsPointer<T>::Value == false &&
	TModels<CMemberToStringConvertable, T>::Value
>::Type>
FString LexToString(const T& Object)
{
	return Object.ToString();
}
