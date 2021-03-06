// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Templates/UnrealTypeTraits.h"
#include "Traits/IsVoidType.h"

namespace OUUAddressOf_Private
{
	template<typename T>
	struct TIsReferencable
	{
		static const bool Value = TIsFunction<T>::Value || TIsReferenceType<T>::Value || TIsVoidType<T>::Value;
	};
}

/** Get a pointer with the true address of an object, even if operator& was overloaded */
template <typename T>
typename TEnableIf<OUUAddressOf_Private::TIsReferencable<T>::Value == false, T*>::Type AddressOf(T& Argument) noexcept
{
	return reinterpret_cast<T*>(
		&const_cast<char&>(
			reinterpret_cast<const volatile char&>(Argument)));
}

/** Get a pointer with the true address of an object, even if operator& was overloaded */
template <typename T>
typename TEnableIf<OUUAddressOf_Private::TIsReferencable<T>::Value == true, T*>::Type AddressOf(T& Argument) noexcept
{
	return &Argument;
}
