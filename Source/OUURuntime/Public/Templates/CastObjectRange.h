// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "IteratorUtils.h"
#include "Traits/ConditionalType.h"
#include "Traits/IteratorTraits.h"

template <typename IteratorType, class CastTargetType>
class TCastObjectIterator
{
private:
	using ElementType = typename TIteratorTraits<IteratorType>::ElementType;
	using CastElementType = typename TConditionalType<
		TIsConst<typename TRemovePointer<ElementType>::Type>::Value,
		const CastTargetType,
		CastTargetType>::Type;

	using PointerType = typename TIteratorTraits<IteratorType>::PointerType;
	using ReferenceType = typename TIteratorTraits<IteratorType>::ReferenceType;

	static_assert(TIsPointer<CastTargetType>::Value == false, "TargetType must not be a pointer type");
	static_assert(UECasts_Private::TIsCastable<CastTargetType>::Value, "TargetType must be a castable UObject type!");

	IteratorType WrappedIterator;

public:
	constexpr TCastObjectIterator() : WrappedIterator() {}
	constexpr explicit TCastObjectIterator(IteratorType It) : WrappedIterator(It) {}

	constexpr CastElementType* operator*() const { return Cast<CastElementType>(*WrappedIterator); }

	// preincrement
	constexpr TCastObjectIterator& operator++()
	{
		++WrappedIterator;
		return (*this);
	}

	// postincrement
	constexpr TCastObjectIterator operator++(int)
	{
		TCastObjectIterator Temp = *this;
		++WrappedIterator;
		return Temp;
	}

	constexpr bool operator==(const TCastObjectIterator& Other) { return WrappedIterator == Other.WrappedIterator; }

	constexpr bool operator!=(const TCastObjectIterator& Other) { return WrappedIterator != Other.WrappedIterator; }
};

template <typename CastTargetType, typename IteratorType>
constexpr auto CreateCastObjectIterator(IteratorType Iterator)
{
	return TCastObjectIterator<IteratorType, CastTargetType>(Iterator);
}

template <
	typename ContainerType,
	typename CastTargetType,
	typename BeginIteratorType = decltype(OUU::Runtime::Private::IteratorUtils::begin(DeclVal<ContainerType>())),
	typename EndIteratorType = decltype(OUU::Runtime::Private::IteratorUtils::end(DeclVal<ContainerType>()))>
class TCastObjectRangeAdaptor
{
private:
	ContainerType Container;

public:
	constexpr explicit TCastObjectRangeAdaptor(ContainerType c) : Container(c) {}

#define DECLARE_RANGED_FOR_OPERATOR(Operator, OptionalConst)                                                           \
	auto Operator() OptionalConst noexcept                                                                             \
	{                                                                                                                  \
		return CreateCastObjectIterator<CastTargetType>(OUU::Runtime::Private::IteratorUtils::Operator(Container));    \
	}
	DECLARE_RANGED_FOR_OPERATOR(begin, PREPROCESSOR_NOTHING);
	DECLARE_RANGED_FOR_OPERATOR(begin, const);
	DECLARE_RANGED_FOR_OPERATOR(end, PREPROCESSOR_NOTHING);
	DECLARE_RANGED_FOR_OPERATOR(end, const);
#undef DECLARE_RANGED_FOR_OPERATOR
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
template <class CastTargetType, typename ContainerType>
constexpr auto CastObjectRange(ContainerType& Container)
{
	return TCastObjectRangeAdaptor<ContainerType&, CastTargetType>(Container);
}

template <class CastTargetType, typename ContainerType>
constexpr auto CastObjectRange(ContainerType&& Container)
{
	return TCastObjectRangeAdaptor<ContainerType, CastTargetType>(MoveTemp(Container));
}
