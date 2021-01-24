// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Templates/IsPointer.h"
#include "Templates/Models.h"
#include "EngineUtils.h"

template<typename IteratorType, bool bIsPointerType = TIsPointer<IteratorType>::Value>
struct TIteratorTraits;

// specialization for non-pointer containers (e.g. TArray, TMap, etc.)
template<typename IteratorType>
struct TIteratorTraits<IteratorType, false>
{
	using ReferenceType = decltype(DeclVal<IteratorType>().operator*());
	using ElementType = typename TRemoveReference<ReferenceType>::Type;
	using PointerType = ElementType*;
};

// specialization for pointers and c-arrays
template<typename IteratorType>
struct TIteratorTraits<IteratorType, true>
{
	using ElementType = typename TRemovePointer<IteratorType>::Type;
	using PointerType = ElementType*;
	using ReferenceType = ElementType&;
};

// Concept for iterator that is bidirectional, i.e. one that can be incremented and decremented.
// See "Models.h"
struct CBidirectionalIterator
{
	template<typename IteratorType>
	auto Requires(IteratorType It) -> decltype(
		--It,
		++It,
		*It,
		TIteratorTraits<IteratorType>::ElementType(*It)
		);
};

// To check if the CBidirectionalIterator concept works as expected, we test TActorIterator, which cannot be decremented
static_assert(TModels<CBidirectionalIterator, TActorIterator<AActor>>::Value == false, "Actor iterator is not bidirectional!");
