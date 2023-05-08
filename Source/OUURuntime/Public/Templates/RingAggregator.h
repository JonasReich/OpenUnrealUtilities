// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CircularAggregator.h"

template <typename ElementType>
class UE_DEPRECATED(4.27, "TRingAggregator has been renamed to TCircularAggregator for consistency") TRingAggregator :
	public TCircularAggregator<ElementType>
{
public:
	TRingAggregator() {}
	TRingAggregator(int32 MaxNum) : TCircularAggregator<ElementType>(MaxNum) {}
};

template <typename ElementType, uint32 ElementNum>
class UE_DEPRECATED(4.27, "TRingAggregator has been renamed to TCircularAggregator for consistency")
	TFixedSizeRingAggregator : public TFixedSizeCircularAggregator<ElementType, ElementNum>
{
public:
	TFixedSizeRingAggregator() : TFixedSizeCircularAggregator<ElementType, ElementNum>() {}
};
