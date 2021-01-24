// Copyright (c) 2021 Jonas Reich

#include "FlowControl/LockTestHelpers.h"
#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "FlowControl/ExclusiveLock.h"
#include "FlowControl/SharedLock.h"
#include "EditorLevelLibrary.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities.FlowControl

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

	TestTrue("lock delegate was called at least once", Env.Helper->NumDelegateCalls > 0);
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
	
	TestTrue("lock delegate was called at least once", Env.Helper->NumDelegateCalls > 0);
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

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(TryLockForDuration, DEFAULT_OUU_TEST_FLAGS)
{
	auto Env = MakeShared<FOUULockTestEnvironment<UExclusiveLock>>(UEditorLevelLibrary::GetEditorWorld());
	Env->Lock->TryLockForDuration(Env->Key0, 6.f);
	TestTrue("Is locked at beginning", Env->Lock->IsLocked());

	AddCommand(new FDelayedFunctionLatentCommand([=]() {
		TestTrue("Is locked after half time", Env->Lock->IsLocked());
	}, 3.f));

	AddCommand(new FDelayedFunctionLatentCommand([=]() {
		TestFalse("Is locked after full time", Env->Lock->IsLocked());
	}, 3.1f));

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
