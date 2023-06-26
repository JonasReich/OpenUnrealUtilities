// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Misc/ScopeRWLock.h"
#include "Traits/ConditionalType.h"

/**
 * Base type for the TRWLockedVariable template that allows access to the lock member without having to know the
 * template types.
 */
class FRWLockedVariable_Base
{
protected:
	template <typename...>
	friend class TScopedMultiRWLock;

	// Lock must be mutable, so it can be locked/unlocked even when accessing it in const context when acquiring
	// read-lock
	mutable FRWLock Lock;
};

// Forward declaration
template <typename VariableType, bool bInIsWriteLock>
class TScopedRWLockedVariableRef;

/**
 * Container for a variable that has a read/write lock.
 * Only allows access to the variable via methods that acquire the corresponding scoped lock.
 */
template <typename InVariableType>
class TRWLockedVariable : public FRWLockedVariable_Base
{
public:
	using VariableType = InVariableType;

	friend class FRWLockedVariableSpec;

	TRWLockedVariable() = default;
	TRWLockedVariable(VariableType InVariableValue) : Variable(InVariableValue) {}

	/** Acquire a scoped read lock reference to the variable */
	auto Read() const
	{
		// Use const VariableType so the resulting ref is a const reference
		return TScopedRWLockedVariableRef<const VariableType, false>(Variable, Lock);
	}

	/** Acquire a scoped write lock reference to the variable */
	auto Write() { return TScopedRWLockedVariableRef<VariableType, true>(Variable, Lock); }

	/**
	 * Get a reference to the underlying variable.
	 * This does not lock the variable, so USE WITH CAUTION!
	 */
	FORCEINLINE const VariableType& GetRefWithoutLocking_USE_WITH_CAUTION() const { return Variable; }

	/**
	 * Get a reference to the underlying variable.
	 * This does not lock the variable, so USE WITH CAUTION!
	 */
	FORCEINLINE VariableType& GetRefWithoutLocking_USE_WITH_CAUTION() { return Variable; }

private:
	VariableType Variable = {};
};

/** Statically assert that a const reference obtained via TRWLockedVariable::Read() cannot be ... */
#define ASSERT_CONST_REF_CANT(Operation)                                                                               \
	static_assert(                                                                                                     \
		std::is_same_v<ScopeLockType, FReadScopeLock> == false,                                                        \
		"TRWVarRef obtained via TRWLockedVariable::Read() is a constant reference to the underlying variable and "     \
		"cannot " Operation);

/**
 * Reference wrapper for a value from a TRWLockedVariable that locks the variable for the lifetime of this object.
 * Provides access to the underlying value via operator overloads and Get() method.
 * Statically asserts that a read lock is not used to modify the value.
 */
template <typename VariableType, bool bInIsWriteLock>
class TScopedRWLockedVariableRef
{
public:
	static const bool bIsWriteLock = bInIsWriteLock;
	// Native scope lock type used internally to lock/release the RWLock
	using ScopeLockType = typename TConditionalType<bIsWriteLock, FWriteScopeLock, FReadScopeLock>::Type;

	template <typename>
	friend class TRWLockedVariable;

	TScopedRWLockedVariableRef(TScopedRWLockedVariableRef&& Other) noexcept :
		VariableRef(Other.VariableRef), Lock(Other.Lock), LockType(Other.LockType)
	{
	}

	VariableType& Get() const { return VariableRef; }
	VariableType& operator->() const { return Get(); }
	VariableType& operator*() const { return Get(); }

	// ---------------------
	// Assignment operators
	// ---------------------

	template <typename T>
	VariableType& operator=(T Other)
	{
		ASSERT_CONST_REF_CANT("be assigned to");
		return Get() = Other;
	}

	// ---------------------
	// Arithmetic operators
	// ---------------------

	VariableType& operator+(const VariableType& OtherVariableValueRef)
	{
		ASSERT_CONST_REF_CANT("be added to");
		return Get() + OtherVariableValueRef;
	}

	VariableType& operator-(const VariableType& OtherVariableValueRef)
	{
		ASSERT_CONST_REF_CANT("be subtracted from");
		return Get() - OtherVariableValueRef;
	}

	VariableType& operator*(const VariableType& OtherVariableValueRef)
	{
		ASSERT_CONST_REF_CANT("be multiplied with");
		return Get() * OtherVariableValueRef;
	}

	VariableType& operator/(const VariableType& OtherVariableValueRef)
	{
		ASSERT_CONST_REF_CANT("be divided");
		return Get() / OtherVariableValueRef;
	}

	// ---------------------
	// Comparison operators
	// ---------------------

	bool operator==(const VariableType& OtherVariableValueRef) const { return Get() == OtherVariableValueRef; }

	bool operator!=(const VariableType& OtherVariableValueRef) const { return Get() != OtherVariableValueRef; }

	bool operator<=(const VariableType& OtherVariableValueRef) const { return Get() <= OtherVariableValueRef; }

	bool operator>=(const VariableType& OtherVariableValueRef) const { return Get() >= OtherVariableValueRef; }

	bool operator<(const VariableType& OtherVariableValueRef) const { return Get() < OtherVariableValueRef; }

	bool operator>(const VariableType& OtherVariableValueRef) const { return Get() > OtherVariableValueRef; }

private:
	TScopedRWLockedVariableRef(VariableType& InVariableRef, FRWLock& InLock) :
		VariableRef(InVariableRef), Lock(InLock), LockType()
	{
	}

	// Reference to the variable value
	VariableType& VariableRef;

	// Reference to the lock
	FRWLock& Lock;

	// LockType for RWScopeLocks
	FRWScopeLockType LockType;
};

#undef ASSERT_CONST_REF_CANT
