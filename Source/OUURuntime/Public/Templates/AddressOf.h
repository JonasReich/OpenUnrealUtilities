// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Templates/UnrealTypeTraits.h"

namespace OUUAddressOf_Private
{
	template <typename T>
	struct TIsReferencable
	{
		static const bool Value = TIsFunction<T>::Value || TIsReferenceType<T>::Value || std::is_void_v<T>;
	};
} // namespace OUUAddressOf_Private

/** Get a pointer with the true address of an object, even if operator& was overloaded */
template <typename T>
typename TEnableIf<OUUAddressOf_Private::TIsReferencable<T>::Value == false, T*>::Type AddressOf(T& Argument) noexcept
{
	return reinterpret_cast<T*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(Argument)));
}

/** Get a pointer with the true address of an object, even if operator& was overloaded */
template <typename T>
typename TEnableIf<OUUAddressOf_Private::TIsReferencable<T>::Value == true, T*>::Type AddressOf(T& Argument) noexcept
{
	return &Argument;
}
