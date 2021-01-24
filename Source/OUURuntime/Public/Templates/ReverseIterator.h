// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Templates/IteratorUtils.h"
#include "Traits/IteratorTraits.h"

/**
 * Wraps a bidirectional iterator and reverses increment/decrement operations.
 * Equivalent of std::reverse_iterator with the following differences:
 * - Omitted construction from compatible base iterator because UE4 iterators don't have a .base() member
 * - Omitted additional operators for random access iterators (+, -, +=, -=, [])
 * - Omitted comparison operators (<, >, >=, <=)
 * - Omitted operators for addition/subtraction of two iterators
 */
template<typename IteratorType>
class TReverseIterator
{
private:
	using ElementType = typename TIteratorTraits<IteratorType>::ElementType;
	using PointerType = typename TIteratorTraits<IteratorType>::PointerType;
	using ReferenceType = typename TIteratorTraits<IteratorType>::ReferenceType;

	static_assert(TModels<CBidirectionalIterator, IteratorType>::Value, "IteratorType does not support bidirectional usage via operator++ and operator--.");

	IteratorType WrappedIterator;
public:
	CONSTEXPR TReverseIterator() : WrappedIterator() {}
	CONSTEXPR explicit TReverseIterator(IteratorType It) : WrappedIterator(It) {}

	CONSTEXPR ReferenceType operator*() const
	{
		IteratorType Temp = WrappedIterator;
		--Temp;
		return *Temp;
	}

	CONSTEXPR PointerType operator->() const
	{
		IteratorType Temp = WrappedIterator;
		--Temp;
		return IteratorUtils::OperatorArrow(Temp);
	}

	// preincrement
	CONSTEXPR TReverseIterator& operator++()
	{
		--WrappedIterator;
		return (*this);
	}

	// postincrement
	CONSTEXPR TReverseIterator operator++(int)
	{
		TReverseIterator Temp = *this;
		--WrappedIterator;
		return Temp;
	}

	// predecrement
	CONSTEXPR TReverseIterator& operator--()
	{
		--WrappedIterator;
		return (*this);
	}

	// postdecrement
	CONSTEXPR TReverseIterator operator--(int)
	{
		TReverseIterator Temp = *this;
		++WrappedIterator;
		return Temp;
	}

	CONSTEXPR bool operator==(const TReverseIterator& Other)
	{
		return WrappedIterator == Other.WrappedIterator;
	}

	CONSTEXPR bool operator!=(const TReverseIterator& Other)
	{
		return WrappedIterator != Other.WrappedIterator;
	}
};

/**
 * Create a reverse iterator from a bidirectional iterator instance.
 * Equivalent to std::make_reverse_iterator()
 */
template<typename IteratorType>
CONSTEXPR auto MakeReverseIterator(IteratorType Iterator)
{
	return TReverseIterator<IteratorType>(Iterator);
}

/** Swaps begin() and end() iterators to allow for reversed iteration over a container, */
template<typename ContainerType>
class TReverseRangeAdaptor_ByRef
{
private:
	ContainerType& Container;
public:
	CONSTEXPR explicit TReverseRangeAdaptor_ByRef(ContainerType& c) : Container(c) {}

	auto begin() const noexcept { return MakeReverseIterator(IteratorUtils::end(Container)); }
	auto end()   const noexcept { return MakeReverseIterator(IteratorUtils::begin(Container)); }
};

/** Swaps begin() and end() iterators to allow for reversed iteration over a container, */
template<typename ContainerType>
class TReverseRangeAdaptor_ByValue
{
private:
	ContainerType Container;
public:
	CONSTEXPR explicit TReverseRangeAdaptor_ByValue(ContainerType&& c) : Container(c) {}

	auto begin() const noexcept { return MakeReverseIterator(IteratorUtils::end(Container)); }
	auto end()   const noexcept { return MakeReverseIterator(IteratorUtils::begin(Container)); }
};

/**	Adapts the referenced container so it can be iterated in reverse with a ranged-for-loop. */
template<typename ContainerType>
CONSTEXPR auto ReverseRange(ContainerType& Container)
{
	return TReverseRangeAdaptor_ByRef<ContainerType>(Container);
}

template<typename ContainerType>
CONSTEXPR auto ReverseRange(ContainerType&& Container)
{
	return TReverseRangeAdaptor_ByValue<ContainerType>(MoveTemp(Container));
}
