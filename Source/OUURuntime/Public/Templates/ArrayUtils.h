// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Containers/Array.h"

class OUURUNTIME_API FArrayUtils
{
public:
	/** Set all existing elements in an array to the same value */
	template<typename ElementType, typename AllocatorType>
	static void SetAllTo(TArray<ElementType, AllocatorType>& Array, const ElementType& Value)
	{
		for(int32 i = 0; i < Array.Num(); i++)
		{
			Array[i] = Value;
		}
	}

	/**
	 * Change an array to the specified length (same as the SetNum() member function)
	 * and assign all elements to the same value.
	 */
	template<typename ElementType, typename AllocatorType>
	static void SetNumTo(TArray<ElementType, AllocatorType>& Array, int32 Num, const ElementType& Value)
	{
		Array.SetNum(Num);
		SetAllTo(Array, Value);
	}
};
