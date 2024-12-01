// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "HAL/Platform.h"
#include "Templates/EnableIf.h"
#include "Templates/IsPointer.h"

/**
 * Contains STL like function templates to use for iterators.
 * These functions are discouraged to use directly outside of iterator utilities.
 */
namespace OUU::Runtime::Private::IteratorUtils
{
	template <class ContainerType>
	constexpr auto begin(ContainerType& Container) -> decltype(Container.begin())
	{
		return (Container.begin());
	}

	template <class ContainerType>
	constexpr auto begin(const ContainerType& Container) -> decltype(Container.begin())
	{
		return (Container.begin());
	}

	template <class ContainerType>
	constexpr auto end(ContainerType& Container) -> decltype(Container.end())
	{
		return (Container.end());
	}

	template <class ContainerType>
	constexpr auto end(const ContainerType& Container) -> decltype(Container.end())
	{
		return (Container.end());
	}

	template <class ElementType, size_t ArraySize>
	constexpr ElementType* begin(ElementType (&CArray)[ArraySize]) noexcept
	{
		return (CArray);
	}

	template <class ElementType, size_t ArraySize>
	constexpr ElementType* end(ElementType (&CArray)[ArraySize]) noexcept
	{
		return (CArray + ArraySize);
	}

	/** Call operator->() on an iterator */
	template <typename IteratorType, typename = typename TEnableIf<TIsPointer<IteratorType>::Value>::Type>
	constexpr IteratorType OperatorArrow(IteratorType&& Target)
	{
		return (Target);
	}

	/** Call operator->() on an iterator */
	template <typename IteratorType, typename = typename TEnableIf<TIsPointer<IteratorType>::Value == false>::Type>
	constexpr IteratorType OperatorArrow(IteratorType Target)
	{
		return Forward(Target).operator->();
	}
} // namespace OUU::Runtime::Private::IteratorUtils
