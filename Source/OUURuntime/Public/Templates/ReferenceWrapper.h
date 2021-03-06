// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Templates/EnableIf.h"
#include "Templates/RemoveReference.h"
#include "Templates/UnrealTemplate.h"
#include "Templates/UnrealTypeTraits.h"
#include "Templates/AddressOf.h"

/**
 * Wrap a reference in a copyable, assignable object.
 * Useful especially to store references in dynamic containers such as TArray, TMap, etc which is not
 * possible with normal references.
 * Reference wrappers can be stale (just like references), but not null or null-checked.
 * They are implicitly convertable from and to T& so they can be used to invoke functions that require T&.
 */
template <typename T>
class TReferenceWrapper
{
public:
	// Type declaration so the type can be used in external type traits.
	using Type = T;

	/** Implicitly construct reference wrappers from anything that allows implicit conversion to T& that is not a reference wrapper */
	template <typename U, typename = typename TEnableIf<!TIsSame<TReferenceWrapper, typename TRemoveReference<U>::Type>::Value>::Type>
	constexpr TReferenceWrapper(U&& Argument)
		: Pointer(AddressOf(Forward<U>(Argument)))
	{
	}

	/**
	 * Allow assignment from other reference wrappers.
	 * Required for many container implementations.
	 */
	TReferenceWrapper(const TReferenceWrapper&) noexcept = default;
	TReferenceWrapper& operator=(const TReferenceWrapper&) noexcept = default;

	/** Implicit access by conversion to T& */
	constexpr operator T&() const noexcept
	{
		return *Pointer;
	}

	/** Explicit access  */
	constexpr T& Get() const noexcept
	{
		return *Pointer;
	}

private:
	T* Pointer = nullptr;
};

/** Helper function to create a reference wrapper from an arbitrary object that is bindable to T& */
template <typename T>
constexpr TReferenceWrapper<T> MakeRef(T& Argument) noexcept
{
	return TReferenceWrapper<T>(Argument);
}

/** Helper function to create a reference wrapper from an arbitrary object that is bindable to T& */
template <typename T>
constexpr TReferenceWrapper<T> MakeRef(TReferenceWrapper<T> Argument) noexcept
{
	return Argument;
}

/** Delete T&& overload, because rvalue-references cannot be bound to lvalue references */
template <typename T>
void MakeRef(const T&&) = delete;

/** Helper function to create a reference wrapper from an arbitrary object that is bindable to const T& */
template <typename T>
constexpr TReferenceWrapper<const T> MakeConstRef(const T& Argument) noexcept
{
	return TReferenceWrapper<const T>(Argument);
}

/** Helper function to create a reference wrapper from an arbitrary object that is bindable to const T& */
template <typename T>
constexpr TReferenceWrapper<const T> MakeConstRef(TReferenceWrapper<T> Argument) noexcept
{
	return TReferenceWrapper<const T>(Argument.Get());
}

/** Helper function to create a reference wrapper from an arbitrary object that is bindable to const T& */
template <typename T>
constexpr TReferenceWrapper<const T> MakeConstRef(TReferenceWrapper<const T> Argument) noexcept
{
	return TReferenceWrapper<const T>(Argument.Get());
}

/** Delete T&& overload, because rvalue-references cannot be bound to lvalue references */
template <typename T>
void MakeConstRef(const T&&) = delete;
