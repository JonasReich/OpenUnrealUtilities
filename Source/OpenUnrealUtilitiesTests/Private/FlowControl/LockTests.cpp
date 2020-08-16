// Copyright (c) 2020 Jonas Reich

#include "FlowControl/LockTests.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "FlowControl/Lock.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities

template<class LockType>
struct FOUULockTestEnvironment
{
	LockType* Lock;
	ULockTestsHelper* Helper;
	UObject* Key0;
	UObject* Key1;

	FOUULockTestEnvironment()
	{
		Lock = NewObject<LockType>();
		Helper = NewObject<ULockTestsHelper>();
		Key0 = NewObject<UOUUTestObject>();
		Key1 = NewObject<UOUUTestObject>();

		AssignLockDelegate(Lock);
	}

	void AssignLockDelegate(UExclusiveLock* InLock)
	{
		InLock->OnLockStateChanged.AddDynamic(Helper, &ULockTestsHelper::HandleExclusiveLockStateChanged);
	}

	void AssignLockDelegate(USharedLock* InLock)
	{
		InLock->OnLockStateChanged.AddDynamic(Helper, &ULockTestsHelper::HandleSharedLockStateChanged);
	}
};

//////////////////////////////////////////////////////////////////////////
// Exclusive Lock tests
//////////////////////////////////////////////////////////////////////////

#define OUU_TEST_TYPE ExclusiveLock

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_Nullptr, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<UExclusiveLock> Env;

	// Act
	bool bLockNullptrSuccess = Env.Lock->TryLock(nullptr);
	bool bIsLockedAfterNullLock = Env.Lock->IsLocked();

	Env.Lock->TryLock(Env.Key0);

	bool bUnlockNullptrSuccess = Env.Lock->TryUnlock(nullptr);
	bool bIsLockedAfterNullUnlock = Env.Lock->IsLocked();

	// Assert

	TestFalse("lock with nullptr successful", bLockNullptrSuccess);
	TestFalse("lock state after nullptr lock attempt", bIsLockedAfterNullLock);
	TestFalse("unlock with nullptr successful", bUnlockNullptrSuccess);
	TestTrue("lock state after nullptr unlock attempt", bIsLockedAfterNullUnlock);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_SingleKey_Once, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<UExclusiveLock> Env;

	// Act
	bool bIsLockedBeforeLocking = Env.Lock->IsLocked();
	bool bLockSuccessful = Env.Lock->TryLock(Env.Key0);
	bool bIsLockedAfterLocking = Env.Lock->IsLocked();
	bool bUnlockSuccessful = Env.Lock->TryUnlock(Env.Key0);
	bool bIsLockedAfterUnlocking = Env.Lock->IsLocked();

	// Assert
	TestFalse("locked before locking", bIsLockedBeforeLocking);
	TestTrue("lock successful", bLockSuccessful);
	TestTrue("lock locked after locking", bIsLockedAfterLocking);
	TestTrue("unlock successful", bUnlockSuccessful);
	TestFalse("lock locked after unlocking", bIsLockedAfterUnlocking);

	TestTrue("lock delegate was called at least once", Env.Helper->bDelegateWasCalled);
	TestTrue("lock passed via callback is correct", Env.Helper->Lock == Env.Lock);
	TestTrue("lock passed via callback always the same", Env.Helper->bLockAlwaysSame);
	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false });

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_SingleKey_TwiceSameTime, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<UExclusiveLock> Env;

	// Act
	bool bIsLockedBeforeLocking = Env.Lock->IsLocked();
	bool bLockSuccessful = Env.Lock->TryLock(Env.Key0);
	bool bIsLockedAfterLocking = Env.Lock->IsLocked();
	bool b2ndLockSuccessful = Env.Lock->TryLock(Env.Key0);
	bool bIsLockedAfter2ndLocking = Env.Lock->IsLocked();
	bool bUnlockSuccessful = Env.Lock->TryUnlock(Env.Key0);
	bool bIsLockedAfterUnlocking = Env.Lock->IsLocked();

	// Assert
	TestFalse("locked before locking", bIsLockedBeforeLocking);
	TestTrue("lock successful", bLockSuccessful);
	TestTrue("lock locked after locking", bIsLockedAfterLocking);
	TestTrue("2nd lock successful", b2ndLockSuccessful);
	TestTrue("lock locked after 2nd locking", bIsLockedAfter2ndLocking);
	TestTrue("unlock successful", bUnlockSuccessful);
	TestFalse("lock locked after unlocking", bIsLockedAfterUnlocking);
	
	TestTrue("lock delegate was called at least once", Env.Helper->bDelegateWasCalled);
	TestTrue("lock passed via callback is correct", Env.Helper->Lock == Env.Lock);
	TestTrue("lock passed via callback always the same", Env.Helper->bLockAlwaysSame);
	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false });

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_SingleKey_TwiceSequential, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<UExclusiveLock> Env;

	// Act
	bool bAllOperationsSuccessful = true;
	bAllOperationsSuccessful |= Env.Lock->TryLock(Env.Key0);
	bAllOperationsSuccessful |= Env.Lock->TryUnlock(Env.Key0);
	bAllOperationsSuccessful |= Env.Lock->TryLock(Env.Key0);
	bAllOperationsSuccessful |= Env.Lock->TryUnlock(Env.Key0);
	bool bIsLockedAfter2ndUnlocking = Env.Lock->IsLocked();

	// Assert
	TestTrue("all operations successful", bAllOperationsSuccessful);
	TestFalse("lock locked after 2nd unlocking", bIsLockedAfter2ndUnlocking);

	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false, true, false });

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_DifferentKey, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<UExclusiveLock> Env;

	// Act
	Env.Lock->TryLock(Env.Key0);
	Env.Lock->IsLocked();
	
	bool bIs2ndKeyLockSuccessful = Env.Lock->TryLock(Env.Key1);
	bool bIsLockedAfter2ndKeyLock = Env.Lock->IsLocked();

	bool bUnlock2ndKeySuccessful = Env.Lock->TryUnlock(Env.Key1);
	bool bIsLockedAfter2ndKeyUnlocking = Env.Lock->IsLocked();

	bool bUnlock1stKeySuccessful = Env.Lock->TryUnlock(Env.Key0);
	bool bIsLockedAfter1stKeyUnlocking = Env.Lock->IsLocked();

	// Assert
	TestFalse("locking with 2nd key successful", bIs2ndKeyLockSuccessful);
	TestTrue("locked after 2nd key lock attempt", bIsLockedAfter2ndKeyLock);
	TestFalse("unlocking with 2nd key successful", bUnlock2ndKeySuccessful);
	TestTrue("locked after 2nd key unlock attempt", bIsLockedAfter2ndKeyUnlocking);
	TestTrue("unlocking with 1st key successful", bUnlock1stKeySuccessful);
	TestFalse("locked after 1st key unlock attempt", bIsLockedAfter1stKeyUnlocking);

	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false });

	return true;
}

#undef OUU_TEST_TYPE

//////////////////////////////////////////////////////////////////////////
// Shared Lock tests
//////////////////////////////////////////////////////////////////////////

#define OUU_TEST_TYPE SharedLock

PRAGMA_DISABLE_OPTIMIZATION

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_Nullptr, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<USharedLock> Env;

	// Act
	Env.Lock->Lock(nullptr);
	bool bIsLockedAfterNullLock = Env.Lock->IsLocked();

	Env.Lock->Lock(Env.Key0);

	bool bUnlockNullptrSuccess = Env.Lock->TryRelease(nullptr);
	bool bIsLockedAfterNullUnlock = Env.Lock->IsLocked();

	// Assert

	TestFalse("lock state after nullptr lock attempt", bIsLockedAfterNullLock);
	TestFalse("unlock with nullptr successful", bUnlockNullptrSuccess);
	TestTrue("lock state after nullptr unlock attempt", bIsLockedAfterNullUnlock);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_SingleKey_Once, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<USharedLock> Env;

	// Act
	bool bIsLockedBeforeLocking = Env.Lock->IsLocked();
	Env.Lock->Lock(Env.Key0);
	bool bIsLockedAfterLocking = Env.Lock->IsLocked();
	bool bUnlockSuccessful = Env.Lock->TryRelease(Env.Key0);
	bool bIsLockedAfterUnlocking = Env.Lock->IsLocked();

	// Assert
	TestFalse("locked before locking", bIsLockedBeforeLocking);
	TestTrue("lock locked after locking", bIsLockedAfterLocking);
	TestTrue("unlock successful", bUnlockSuccessful);
	TestFalse("lock locked after unlocking", bIsLockedAfterUnlocking);

	TestTrue("lock delegate was called at least once", Env.Helper->bDelegateWasCalled);
	TestTrue("lock passed via callback is correct", Env.Helper->Lock == Env.Lock);
	TestTrue("lock passed via callback always the same", Env.Helper->bLockAlwaysSame);
	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false });

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_SingleKey_TwiceSameTime, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<USharedLock> Env;

	// Act
	bool bIsLockedBeforeLocking = Env.Lock->IsLocked();
	Env.Lock->Lock(Env.Key0);
	bool bIsLockedAfterLocking = Env.Lock->IsLocked();
	Env.Lock->Lock(Env.Key0);
	bool bIsLockedAfter2ndLocking = Env.Lock->IsLocked();
	bool bUnlockSuccessful = Env.Lock->TryRelease(Env.Key0);
	bool bIsLockedAfterUnlocking = Env.Lock->IsLocked();

	// Assert
	TestFalse("locked before locking", bIsLockedBeforeLocking);
	TestTrue("lock locked after locking", bIsLockedAfterLocking);
	TestTrue("lock locked after 2nd locking", bIsLockedAfter2ndLocking);
	TestTrue("unlock successful", bUnlockSuccessful);
	TestFalse("lock locked after unlocking", bIsLockedAfterUnlocking);

	TestTrue("lock delegate was called at least once", Env.Helper->bDelegateWasCalled);
	TestTrue("lock passed via callback is correct", Env.Helper->Lock == Env.Lock);
	TestTrue("lock passed via callback always the same", Env.Helper->bLockAlwaysSame);
	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false });

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_SingleKey_TwiceSequential, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<USharedLock> Env;

	// Act
	bool bAllOperationsSuccessful = true;
	Env.Lock->Lock(Env.Key0);
	bAllOperationsSuccessful |= Env.Lock->TryRelease(Env.Key0);
	Env.Lock->Lock(Env.Key0);
	bAllOperationsSuccessful |= Env.Lock->TryRelease(Env.Key0);
	bool bIsLockedAfter2ndUnlocking = Env.Lock->IsLocked();

	// Assert
	TestTrue("all operations successful", bAllOperationsSuccessful);
	TestFalse("lock locked after 2nd unlocking", bIsLockedAfter2ndUnlocking);

	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false, true, false });

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LockUnlock_DifferentKey, DEFAULT_OUU_TEST_FLAGS)
{
	// Arrange
	FOUULockTestEnvironment<USharedLock> Env;

	// Act
	Env.Lock->Lock(Env.Key0);

	TArray<UObject*> KeysAfterFirstKeySet = Env.Lock->GetActiveKeys();

	Env.Lock->Lock(Env.Key1);
	bool bIsLockedAfter2ndKeyLock = Env.Lock->IsLocked();
	TArray<UObject*> KeysAfterSecondKeySet = Env.Lock->GetActiveKeys();

	bool bUnlock2ndKeySuccessful = Env.Lock->TryRelease(Env.Key1);
	bool bIsLockedAfter2ndKeyUnlocking = Env.Lock->IsLocked();
	TArray<UObject*> KeysAfter2ndKeyRemoved = Env.Lock->GetActiveKeys();

	bool bUnlock1stKeySuccessful = Env.Lock->TryRelease(Env.Key0);
	bool bIsLockedAfter1stKeyUnlocking = Env.Lock->IsLocked();
	TArray<UObject*> KeysAfter1stKeyRemoved = Env.Lock->GetActiveKeys();

	// Assert
	TestTrue("locked after 2nd key lock attempt", bIsLockedAfter2ndKeyLock);
	TestFalse("unlocking with 2nd key successful", bUnlock2ndKeySuccessful);
	TestTrue("locked after 2nd key unlock attempt", bIsLockedAfter2ndKeyUnlocking);
	TestTrue("unlocking with 1st key successful", bUnlock1stKeySuccessful);
	TestFalse("locked after 1st key unlock attempt", bIsLockedAfter1stKeyUnlocking);

	TestArraysEqual(*this, "lock state history", Env.Helper->LockStateHistory, { true, false });

	TestUnorderedArraysMatch(*this, "KeysAfterFirstKeySet", KeysAfterFirstKeySet, {Env.Key0});
	TestUnorderedArraysMatch(*this, "KeysAfterSecondKeySet", KeysAfterSecondKeySet, { Env.Key0, Env.Key1 });
	TestUnorderedArraysMatch(*this, "KeysAfterSecondKeyRemoved", KeysAfter2ndKeyRemoved, { Env.Key0 });
	TestUnorderedArraysMatch(*this, "KeysAfterFirstKeyRemoved", KeysAfter1stKeyRemoved, { });

	return true;
}


//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
