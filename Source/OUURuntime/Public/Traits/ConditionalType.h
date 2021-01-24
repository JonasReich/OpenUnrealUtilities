// Copyright (c) 2021 Jonas Reich

#pragma once

/** Static switch for types: Select type based on Condition */
template <bool Condition, typename TrueType, typename FalseType>
struct TConditionalType;

template <typename TrueType, typename FalseType>
struct TConditionalType<true, TrueType, FalseType>
{
	using Type = TrueType;
};

template <typename TrueType, typename FalseType>
struct TConditionalType<false, TrueType, FalseType>
{
	using Type = FalseType;
};
