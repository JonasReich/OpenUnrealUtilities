// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Algo/MaxElement.h"
#include "Algo/MinElement.h"
#include "CircularArrayAdaptor.h"
#include "Containers/Array.h"
#include "Traits/IsInteger.h"

/**
 * Ring buffer that aggregates numeric data.
 * Useful e.g. for tracking data of the last X frames.
 */
template <class InChildClass, typename InElementType, typename InAllocatorType>
class TCircularAggregator_Base : public TCircularArrayAdaptor_Base<InChildClass, InElementType, InAllocatorType>
{
public:
	using ChildClass = InChildClass;
	using ElementType = InElementType;
	using AllocatorType = InAllocatorType;
	using Super = TCircularArrayAdaptor_Base<ChildClass, InElementType, AllocatorType>;
	using ArrayType = TArray<ElementType, AllocatorType>;

	explicit TCircularAggregator_Base(int32 MaxNum) : Super(Storage, MaxNum), Storage({})
	{
		check(MaxNum > 0);
		Super::StorageReference = Storage;
		Storage.Reserve(MaxNum);
		check(&Storage == &GetStorage());
		check(Storage == GetStorage());
	}
	TCircularAggregator_Base() : TCircularAggregator_Base(32) {}

	TCircularAggregator_Base(const TCircularAggregator_Base& Other) : TCircularAggregator_Base() { *this = Other; }
	TCircularAggregator_Base(TCircularAggregator_Base&& Other) noexcept : TCircularAggregator_Base()
	{
		*this = MoveTemp(Other);
	}
	TCircularAggregator_Base& operator=(const TCircularAggregator_Base& Other)
	{
		*(static_cast<Super*>(this)) = Other;
		Storage = Other.Storage;
		Super::StorageReference = Storage;
		return *this;
	}
	TCircularAggregator_Base& operator=(TCircularAggregator_Base&& Other) noexcept
	{
		*(static_cast<Super*>(this)) = MoveTemp(Other);
		Storage = MoveTemp(Other.Storage);
		Super::StorageReference = Storage;
		return *this;
	}

	ElementType Sum() const
	{
		ElementType ResultSum = 0;
		for (ElementType Element : Storage)
		{
			AddNumbersEnsured<ElementType>(ResultSum, Element);
		}
		return ResultSum;
	}

	ElementType Average() const { return Super::HasData() ? (Sum() / Super::Num()) : 0; }

	ElementType Max() const { return Super::HasData() ? (*Algo::MaxElement(Storage)) : 0; }

	ElementType Min() const { return Super::HasData() ? (*Algo::MinElement(Storage)) : 0; }

	const TArray<ElementType, AllocatorType>& GetStorage() const { return Storage; }

protected:
	TArray<ElementType, AllocatorType> Storage = {};

	/** Overload for builds with ensure macros that checks for integer overflow */
	template <typename T, typename = typename TEnableIf<TIsInteger<T>::Value&& static_cast<bool>(DO_CHECK)>::Type>
	static void AddNumbersEnsured(T& A, T B)
	{
		ElementType SignBefore = FMath::Sign(A);
		A += B;
		ElementType SignAfter = FMath::Sign(A);
		ensureMsgf(
			SignBefore == SignAfter || ((SignBefore + SignAfter) != 0 || SignBefore != FMath::Sign(B)),
			TEXT("integer overflow detected"));
	}

	template <
		typename T,
		typename = typename TEnableIf<TIsInteger<T>::Value == false || static_cast<bool>(DO_CHECK) == false>::Type>
	static void AddNumbersEnsured(T& A, T B, int32 = 0)
	{
		A += B;
	}
};

/** "Normal" ring aggregator that can be initialized with different size depending on dynamic conditions */
template <typename ElementType>
class TCircularAggregator :
	public TCircularAggregator_Base<TCircularAggregator<ElementType>, ElementType, FDefaultAllocator>
{
public:
	using Super = TCircularAggregator_Base<TCircularAggregator<ElementType>, ElementType, FDefaultAllocator>;
	TCircularAggregator() = default;
	explicit TCircularAggregator(int32 MaxNum) : Super(MaxNum) {}
};

/** Ring aggregator that has a compile time fixed size of elements */
template <typename ElementType, uint32 ElementNum>
class TFixedSizeCircularAggregator :
	public TCircularAggregator_Base<
		TFixedSizeCircularAggregator<ElementType, ElementNum>,
		ElementType,
		TFixedAllocator<ElementNum>>
{
public:
	using Super = TCircularAggregator_Base<
		TFixedSizeCircularAggregator<ElementType, ElementNum>,
		ElementType,
		TFixedAllocator<ElementNum>>;
	TFixedSizeCircularAggregator() : Super(ElementNum) {}
	// no constructor with MaxNum parameter, because it's a compile time constant ("ElementNum")
};
