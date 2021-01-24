// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Templates/Tuple.h"

namespace AesirTie_Private
{
	/**
	 * Wrapper struct used solely for c++ std style tie function Tie()
	 * std::tuple<Ts&...> is assignable from std::tuple<Ts...>, however
	 * the UE4 TTuple does not support this assignment conversion.
	 * This behavior is required for std::tie(). To fix this without modifying
	 * the engine source, we created this wrapper class that extends TTuple and
	 * adds the corresponding assignment operator.
	 */
	template <typename... Types>
	struct TTieTupleWrapper : TTuple<Types&...>
	{
		// The type that we wrapped
		typedef TTuple<Types&...> Super;

		// The type that we want to support assignments from
		typedef TTuple<Types...> AssignFromType;

		// Wrap super constructor. This should be the only available constructor.
		TTieTupleWrapper(Types&... args) : Super(args...) {}

		TTieTupleWrapper() = default;
		TTieTupleWrapper(TTieTupleWrapper&&) = default;
		TTieTupleWrapper(const TTieTupleWrapper&) = default;
		TTieTupleWrapper& operator=(TTieTupleWrapper&&) = default;
		TTieTupleWrapper& operator=(const TTieTupleWrapper&) = default;

		// Copy from tuples with non-ref values
		TTieTupleWrapper& operator=(const AssignFromType& Other)
		{
			VisitTupleElements([](auto& A, const auto& B) {
				A = B;
			}, *this, Other);
			return *this;
		}

		// Move from tuples with non-ref values
		TTieTupleWrapper& operator=(AssignFromType&& Other)
		{
			VisitTupleElements([](auto& A, auto&& B) {
				A = MoveTemp(B);
			}, *this, Other);
			return *this;
		}
	};
}

namespace UE4Tuple_Private
{
	// Extend template from UE4 engine source so we also have a TupleArity defined
	// for TTieTupleWrapper not just for TTuple
	template <typename... Types>
	struct TCVTupleArity<const volatile AesirTie_Private::TTieTupleWrapper<Types...>>
	{
		enum { Value = sizeof...(Types) };
	};
}

/**
 * Creates a tuple of lvalue references to its arguments.
 * See AesirUtilitiesTets/Private/TieTests.cpp for example usages.
 */
template <typename... Types>
FORCEINLINE AesirTie_Private::TTieTupleWrapper<Types...> Tie(Types&... Args)
{
	return AesirTie_Private::TTieTupleWrapper<Types...>(Args...);
}
