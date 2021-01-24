// Copyright (c) 2021 Jonas Reich

#pragma once

#include "HAL/Platform.h"
#include "Templates/EnableIf.h"
#include "Templates/IsPointer.h"

/**
 * Contains STL like function templates to use for iterators.
 * These functions are discouraged to use directly outside of iterator utilities.
 */
namespace IteratorUtils
{
	template<class ContainerType>
	CONSTEXPR auto begin(ContainerType& Container) -> decltype(Container.begin())
	{
		return (Container.begin());
	}

	template<class ContainerType>
	CONSTEXPR auto begin(const ContainerType& Container) -> decltype(Container.begin())
	{
		return (Container.begin());
	}

	template<class ContainerType>
	CONSTEXPR auto end(ContainerType& Container) -> decltype(Container.end())
	{
		return (Container.end());
	}

	template<class ContainerType>
	CONSTEXPR auto end(const ContainerType& Container) -> decltype(Container.end())
	{
		return (Container.end());
	}

	template<class ElementType, size_t ArraySize>
	CONSTEXPR ElementType* begin(ElementType(&CArray)[ArraySize]) noexcept
	{
		return (CArray);
	}

	template<class ElementType, size_t ArraySize>
	CONSTEXPR ElementType* end(ElementType(&CArray)[ArraySize]) noexcept
	{
		return (CArray + ArraySize);
	}

	/** Call operator->() on an iterator */
	template<typename IteratorType, typename = typename TEnableIf<TIsPointer<IteratorType>::Value>::Type>
	CONSTEXPR IteratorType OperatorArrow(IteratorType&& Target)
	{
		return (Target);
	}

	/** Call operator->() on an iterator */
	template<typename IteratorType, typename = typename TEnableIf<TIsPointer<IteratorType>::Value == false>::Type>
	CONSTEXPR IteratorType OperatorArrow(IteratorType Target)
	{
		return Forward(Target).operator->();
	}
}
