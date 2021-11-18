// Copyright (c) 2021 Jonas Reich

#pragma once

#define ASSERT_CONST_REF_CANT(Operation) \
	static_assert(TIsSame<ScopeLockType, FReadScopeLock>::Value == false, \
	"TRWVarRef obtained via Read() is a constant reference to the underlying variable and cannot " Operation)

template <typename VariableType, typename ScopeLockType>
class TRWVarRef
{
public:
	template <typename>
	friend class TRWLockedVariable;

	TRWVarRef(TRWVarRef&& Other) noexcept :
		VariableRef(Other.VariableRef),
		Lock(Other.Lock),
		ScopedLock(Other.Lock)
	{
	}

	VariableType& Get() const { return VariableRef; }
	VariableType& operator->() const { return Get(); }
	VariableType& operator*() const { return Get(); }

	// assignment
	template<typename T>
	VariableType& operator=(T Other)
	{
		ASSERT_CONST_REF_CANT("be assigned to");
		return Get() = Other;
	}

	// arithmetic operators
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

	// comparison operators
	bool operator==(const VariableType& OtherVariableValueRef) const
	{
		return Get() == OtherVariableValueRef;
	}
	bool operator!=(const VariableType& OtherVariableValueRef) const
	{
		return Get() != OtherVariableValueRef;
	}
	bool operator<=(const VariableType& OtherVariableValueRef) const
	{
		return Get() <= OtherVariableValueRef;
	}
	bool operator>=(const VariableType& OtherVariableValueRef) const
	{
		return Get() >= OtherVariableValueRef;
	}
	bool operator<(const VariableType& OtherVariableValueRef) const
	{
		return Get() < OtherVariableValueRef;
	}
	bool operator>(const VariableType& OtherVariableValueRef) const
	{
		return Get() > OtherVariableValueRef;
	}

private:
	TRWVarRef(VariableType& InVariableRef, FRWLock& InLock) :
		VariableRef(InVariableRef),
		Lock(InLock),
		ScopedLock(InLock)
	{
	}

	VariableType& VariableRef;
	FRWLock& Lock;
	ScopeLockType ScopedLock;
};

#undef ASSERT_CONST_REF_CANT

template <typename VariableType>
class TRWLockedVariable
{
public:
	auto Read()
	{
		// Use const VariableType so the resulting ref is a const reference
		return TRWVarRef<const VariableType, FReadScopeLock>(Variable, Lock);
	}

	auto Write()
	{
		return TRWVarRef<VariableType, FWriteScopeLock>(Variable, Lock);
	}

	auto ReadWrite()
	{
		return TRWVarRef<VariableType, FRWScopeLock>(Variable, Lock);
	}

private:
	VariableType Variable;
	FRWLock Lock;
};
