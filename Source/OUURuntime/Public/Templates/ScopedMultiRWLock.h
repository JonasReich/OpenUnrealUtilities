// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Misc/EngineVersionComparison.h"
#include "Templates/RWLockedVariable.h"
#include "Traits/ConditionalType.h"

/** Implementation details for TScopedMultiRWLock */
namespace OUU_ScopedMultiRWLock_Private
{
	/**
	 * Reference to FRWLockedVariable_Base.
	 * Contains shared info that is required for all read/write locking operations but not for accessing the data.
	 */
	class FScopedMultiRWLockRef_Base
	{
	public:
		template <typename...>
		friend class TScopedMultiRWLock;

		FScopedMultiRWLockRef_Base(FRWLockedVariable_Base& InRWLockVariable_Base_Ref, bool bInIsWriteLock) :
			RWLockVariable_Base_Ref(InRWLockVariable_Base_Ref), bIsWriteLock(bInIsWriteLock)
		{
		}

	private:
		FRWLockedVariable_Base& RWLockVariable_Base_Ref;

		/**
		 * Dynamic info if the type is a write lock (as opposed to a read lock).
		 * Used for actual locking code, because for that we need pointers to this base type.
		 * Value retrieval code can use the static info in TScopedMultiRWLockRef::IsWriteLock instead.
		 */
		bool bIsWriteLock = false;
	};

	/**
	 * Templated reference to a TRWLockedVariable used inside the multi-lock.
	 * This version extends upon FScopedMultiRWLockRef_Base by adding a reference
	 * to the actual TRWLockedVariable which holds the data member.
	 */
	template <typename InVariableType, bool bInIsWriteLock>
	class TScopedMultiRWLockRef : public FScopedMultiRWLockRef_Base
	{
	public:
		using VariableType = InVariableType;
		using RWLockedVariableType = TRWLockedVariable<VariableType>;

		template <typename...>
		friend class TScopedMultiRWLock;

		/**
		 * Static type info that denotes whether the lock is a write lock.
		 * This should always have the same value as FScopedMultiRWLockRef_Base::bIsWriteLock.
		 */
		enum
		{
			IsWriteLock = bInIsWriteLock
		};

		TScopedMultiRWLockRef(RWLockedVariableType& InRWLockedVariable) :
			FScopedMultiRWLockRef_Base(InRWLockedVariable, bInIsWriteLock), RWLockedVariable_Ref(InRWLockedVariable)
		{
		}

	private:
		/**
		 * Reference to the instance of the template instance of TRWLockedVariable.
		 * This way we can access the actual value of the variable.
		 */
		RWLockedVariableType& RWLockedVariable_Ref;
	};
}; // namespace OUU_ScopedMultiRWLock_Private

/**
 * TScopedMultiRWLock allows locking multiple TRWLockedVariable's at the same time and ensure at runtime time that
 * they will be locked in the same order when locking and unlocking from multiple places.
 *
 * LockRefTypes is a variadic template type list consisting of TScopedMultiRWLockRef<> instantiations.
 *
 * Please use the template functions MakeScopedMultiRWLock(), ScopedMultiRWLock_Write() and ScopedMultiRWLock_Read()
 * to avoid having to explicitly specify the template parameter types.
 *
 * Example:
 *
 *    // Declare TRWLockedVariables
 *    TRWLockedVariable<int32> RWLockedInt;
 *    TRWLockedVariable<TArray<int32>> RWLockedArray;
 *
 *    {
 *        // Create scoped multi read/write lock
 *        // -> each variable gets automatically unlocked for read/write appropriately
 *        auto ScopedMultiLock = MakeScopedMultiRWLock(Read(RWLockedArray), Write(RWLockedInt));
 *
 *        // Tie pointers to underlying data to actual variables
 *        int32* IntPtr;
 *        TArray<int32>* ArrayPtr;
 *        Tie(ArrayPtr, IntPtr) = ScopedMultiLock.GetPointers();
 *
 *        // Use pointers
 *        IntPtr = 15;
 *
 *        // Alternatively you can get the references directly using GetByIdx()
 *        int32& IntRef = ScopedMultiLock.GetByIdx<1>();
 *        IntRef = 3;
 *   }
 *   // after the multi-lock ran out of scope the read/write locks are automatically locked again
 */
template <typename... LockRefTypes>
class TScopedMultiRWLock
{
public:
	using FScopedMultiRWLockRef_Base = OUU_ScopedMultiRWLock_Private::FScopedMultiRWLockRef_Base;
	using ThisType = TScopedMultiRWLock<LockRefTypes...>;

	// Number of variables / locks controlled by this multi lock
	static const uint32 NumLocks = sizeof...(LockRefTypes);

	TScopedMultiRWLock(const LockRefTypes&... InLockReferences) : LockReferences(InLockReferences...)
	{
		// Add pointers to the locks to the array
		VisitTupleElements([&](FScopedMultiRWLockRef_Base& LockRef) { LockPointers.Add(&LockRef); }, LockReferences);

		// Sort locks by memory address
		LockPointers.Sort([](const FScopedMultiRWLockRef_Base& Left, const FScopedMultiRWLockRef_Base& Right) -> bool {
			const FScopedMultiRWLockRef_Base* LeftPtr = &Left;
			const FScopedMultiRWLockRef_Base* RightPtr = &Right;
			return LeftPtr < RightPtr;
		});

		// Go through all sorted locks and acquire the appropriate lock
		for (const FScopedMultiRWLockRef_Base* LockRef : LockPointers)
		{
			if (LockRef->bIsWriteLock)
			{
				LockRef->RWLockVariable_Base_Ref.Lock.WriteLock();
			}
			else
			{
				LockRef->RWLockVariable_Base_Ref.Lock.ReadLock();
			}
		}
	}

	~TScopedMultiRWLock()
	{
		// Go through all sorted locks and release the appropriate lock
		for (const FScopedMultiRWLockRef_Base* LockRef : LockPointers)
		{
			if (LockRef->bIsWriteLock)
			{
				LockRef->RWLockVariable_Base_Ref.Lock.WriteUnlock();
			}
			else
			{
				LockRef->RWLockVariable_Base_Ref.Lock.ReadUnlock();
			}
		}
	}

	/**
	 * Get a reference to the underlying data variables referenced by this multi lock by index.
	 * Index order is determined by the order you used in the constructor for creating the scoped multi-lock.
	 */
	template <
		int32 Idx,
		typename LockRefType = typename TTupleElement<Idx, TTuple<LockRefTypes...>>::Type,
		typename VariableType = typename LockRefType::VariableType,
		// return const reference if the type is only read-locked
		typename ResultType =
			typename TConditionalType<LockRefType::IsWriteLock, VariableType, const VariableType>::Type>
	ResultType& GetByIdx() const
	{
		return LockReferences.template Get<Idx>().RWLockedVariable_Ref.GetRefWithoutLocking_USE_WITH_CAUTION();
	}

	/**
	 * Copied from TTransformTuple_Impl and adjusted for our needs.
	 * We do not pass a custom functor to transform the tuple
	 * Instead a reference to the enclosing type is passed so we can call the GetByIdx() implementation above.
	 * This is statically repeated for every index of the tuple.
	 */
	template <typename IntegerSequence>
	struct TTransformTuple_Impl;

	template <uint32... Indices>
	struct TTransformTuple_Impl<TIntegerSequence<uint32, Indices...>>
	{
		static auto Do(const ThisType& TypedThis)
		{
			// Call &TypedThis.GetByIdx<Indices>() for every Index in the parameter pack
#if UE_VERSION_OLDER_THAN(5, 0, 0)
			return UE4Tuple_Private::MakeTupleImpl(&TypedThis.GetByIdx<Indices>()...);
#else
			return UE::Core::Private::Tuple::MakeTupleImpl(&TypedThis.GetByIdx<Indices>()...);
#endif
		}
	};

	/**
	 * Get a tuple of pointers to the underlying data variables referenced by this multi lock.
	 * Can be used in conjunction with Tie() to assign pointers in your function (see usage example in class docs
	 * above). The order of elements is determined by the order you used in the constructor for creating the scoped
	 * multi-lock.
	 *
	 * Please note that pointers obtained for items with read-only access are const!
	 * -> The compile errors when assigning this to a wrong pointer tuple using Tie() are very sparse and hard to read.
	 */
	auto GetPointers() const { return TTransformTuple_Impl<TMakeIntegerSequence<uint32, NumLocks>>::Do(*this); }

private:
	// Pointers to base types sorted by memory address
	TArray<FScopedMultiRWLockRef_Base*, TFixedAllocator<NumLocks>> LockPointers;

	// References to template instances sorted by same order as passed into constructor
	TTuple<LockRefTypes...> LockReferences;
};

/**
 * Create a TScopedMultiRWLock (see above) from a list of TScopedMultiRWLockRef.
 * Those references are created with the Write() and Get() functions below.
 */
template <typename... LockRefTypes>
auto MakeScopedMultiRWLock(LockRefTypes... LockRefs)
{
	return TScopedMultiRWLock<LockRefTypes...>(LockRefs...);
}

/**
 * Mark a TRWLockedVariable for WRITE access when passing to MakeScopedMultiRWLock()
 */
template <typename VariableType>
auto Write(TRWLockedVariable<VariableType>& RWLockedVariable)
{
	return OUU_ScopedMultiRWLock_Private::TScopedMultiRWLockRef<VariableType, true>(RWLockedVariable);
}

/**
 * Mark a TRWLockedVariable for READ access when passing to MakeScopedMultiRWLock()
 */
template <typename VariableType>
auto Read(TRWLockedVariable<VariableType>& RWLockedVariable)
{
	return OUU_ScopedMultiRWLock_Private::TScopedMultiRWLockRef<VariableType, false>(RWLockedVariable);
}
