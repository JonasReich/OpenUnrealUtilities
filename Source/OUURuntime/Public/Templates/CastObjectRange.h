// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Traits/IteratorTraits.h"
#include "Traits/ConditionalType.h"
#include "IteratorUtils.h"


template<typename IteratorType, class CastTargetType>
class TCastObjectIterator
{
private:
	using ElementType = typename TIteratorTraits<IteratorType>::ElementType;
	using CastElementType = typename TConditionalType <TIsConst<typename TRemovePointer<ElementType>::Type>::Value, const CastTargetType, CastTargetType>::Type;

	using PointerType = typename TIteratorTraits<IteratorType>::PointerType;
	using ReferenceType = typename TIteratorTraits<IteratorType>::ReferenceType;

	// #TODO-j.reich Rephrase individual checks into single condition
	// These requirements stem from the usage of Cast<T> in the operator*():
	static_assert(TIsPointer<ElementType>::Value == true, "Array ElementType must be a pointer type");
	static_assert(TIsPointer<CastTargetType>::Value == false, "TargetType must not be a pointer type");
	//static_assert(TIsCastable<typename TRemoveReference<ElementType>::Type>::Value, "ElementType must be a castable UObject type!");
	static_assert(TIsCastable<CastTargetType>::Value, "TargetType must be a castable UObject type!");

	IteratorType WrappedIterator;
public:
	CONSTEXPR TCastObjectIterator() : WrappedIterator() {}
	CONSTEXPR explicit TCastObjectIterator(IteratorType It) : WrappedIterator(It) {}

	CONSTEXPR CastElementType* operator*() const
	{
		return Cast<CastElementType>(*WrappedIterator);
	}

	// preincrement
	CONSTEXPR TCastObjectIterator& operator++()
	{
		++WrappedIterator;
		return (*this);
	}

	// postincrement
	CONSTEXPR TCastObjectIterator operator++(int)
	{
		TCastObjectIterator Temp = *this;
		++WrappedIterator;
		return Temp;
	}

	CONSTEXPR bool operator==(const TCastObjectIterator& Other)
	{
		return WrappedIterator == Other.WrappedIterator;
	}

	CONSTEXPR bool operator!=(const TCastObjectIterator& Other)
	{
		return WrappedIterator != Other.WrappedIterator;
	}
};

template<typename CastTargetType, typename IteratorType>
CONSTEXPR auto CreateCastObjectIterator(IteratorType Iterator)
{
	return TCastObjectIterator<IteratorType, CastTargetType>(Iterator);
}

template<typename ContainerType, typename CastTargetType,
	typename BeginIteratorType = typename decltype(IteratorUtils::begin(DeclVal<ContainerType>())),
	typename EndIteratorType = typename decltype(IteratorUtils::end(DeclVal<ContainerType>()))>
	class TCastObjectRangeAdaptor
{
private:
	ContainerType Container;

public:
	CONSTEXPR explicit TCastObjectRangeAdaptor(ContainerType c) : Container(c) {}

	auto begin() const noexcept { return CreateCastObjectIterator<CastTargetType>(IteratorUtils::begin(Container)); }
	auto end()   const noexcept { return CreateCastObjectIterator<CastTargetType>(IteratorUtils::end(Container)); }
	auto begin()       noexcept { return CreateCastObjectIterator<CastTargetType>(IteratorUtils::begin(Container)); }
	auto end()         noexcept { return CreateCastObjectIterator<CastTargetType>(IteratorUtils::end(Container)); }
};

/**
 * Allows iterating via ranged-for-loop over a container of UObject*
 * can be used like this:
 * TArray<UObject*> Array = ...
 * for(AActor* ValidActor : CastObjectRange<AActor>(Array))
 * {
 *     ...
 * }
 */
template<class CastTargetType, typename ContainerType>
CONSTEXPR auto CastObjectRange(ContainerType& Container)
{
	return TCastObjectRangeAdaptor<ContainerType&, CastTargetType>(Container);
}

template<class CastTargetType, typename ContainerType>
CONSTEXPR auto CastObjectRange(ContainerType&& Container)
{
	return TCastObjectRangeAdaptor<ContainerType, CastTargetType>(MoveTemp(Container));
}
