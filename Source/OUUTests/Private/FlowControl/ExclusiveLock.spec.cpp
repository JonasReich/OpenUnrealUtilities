// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "FlowControl/LockTestHelpers.h"
#include "EditorLevelLibrary.h"

BEGIN_DEFINE_SPEC(FExclusiveLockSpec, "OpenUnrealUtilities.FlowControl.ExclusiveLock", DEFAULT_OUU_TEST_FLAGS)
TSharedPtr<FOUULockTestEnvironment<UExclusiveLock>> Env;
END_DEFINE_SPEC(FExclusiveLockSpec)
void FExclusiveLockSpec::Define()
{
	BeforeEach([this]()
	{
		Env = MakeShared<FOUULockTestEnvironment<UExclusiveLock>>(UEditorLevelLibrary::GetEditorWorld());
	});

	Describe("TryLock", [this]()
	{
		It("should fail to lock the lock if the key object is nullptr", [this]()
		{
			SPEC_TEST_FALSE(Env->Lock->TryLock(nullptr));
			SPEC_TEST_FALSE(Env->Lock->IsLocked());
		});

		It("should succeed to lock the lock if the key object is a valid object", [this]()
		{
			SPEC_TEST_TRUE(Env->Lock->TryLock(Env->Key0));
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});

		It("should succeed without error when called twice", [this]()
		{
			SPEC_TEST_TRUE(Env->Lock->TryLock(Env->Key0));
			SPEC_TEST_TRUE(Env->Lock->TryLock(Env->Key0));
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});

		It("should fail to lock the lock with a second key after it was already locked by a different object", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			SPEC_TEST_FALSE(Env->Lock->TryLock(Env->Key1));
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});
	});

	Describe("TryUnlock", [this]()
	{
		It("should succeed if the lock was not locked before", [this]()
		{
			SPEC_TEST_TRUE(Env->Lock->TryUnlock(Env->Key0));
		});

		It("should succeed if the lock was locked with the same key object", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			SPEC_TEST_TRUE(Env->Lock->TryUnlock(Env->Key0));
			SPEC_TEST_FALSE(Env->Lock->IsLocked());
		});

		It("should fail if the lock was locked with a different object", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			SPEC_TEST_FALSE(Env->Lock->TryUnlock(Env->Key1));
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});

		It("should make the lock lockable with the same key object again", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			Env->Lock->TryLock(Env->Key0);
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
			Env->Lock->TryUnlock(Env->Key0);
			SPEC_TEST_FALSE(Env->Lock->IsLocked());
		});
	});

	Describe("OnLockStateChanged", [this]()
	{
		It("should be called when the lock is locked", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 1);
		});

		It("should be called again when the lock is unlocked", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 2);
		});

		It("should not be called again when the lock state doesn't change (for example when the lock is locked twice in a row)", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			Env->Lock->TryLock(Env->Key1);
			SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 1);
		});

		It("should pass the correct lock and lock state as parameters", [this]()
		{
			Env->Lock->TryLock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			Env->Lock->TryLock(Env->Key1);
			Env->Lock->TryLock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key1);

			SPEC_TEST_TRUE(Env->Helper->bLockAlwaysSame);
			SPEC_TEST_EQUAL(Env->Helper->Lock, Cast<UObject>(Env->Lock));

			SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 4);
			TArray<bool> Bools { true, false, true, false };
			SPEC_TEST_ARRAYS_EQUAL(Env->Helper->LockStateHistory, Bools);
		});
	});

	Describe("TryLockForDuration", [this]()
	{
		LatentIt("should lock the lock for specified duration", [this](const FDoneDelegate& DoneDelegate)
		{
			Env->Lock->TryLockForDuration(Env->Key0, 6.f);

			TestTrue("Is locked at beginning", Env->Lock->IsLocked());

			UWorld* World = UEditorLevelLibrary::GetEditorWorld();
			FTimerHandle InOutHandle;
			World->GetTimerManager().SetTimer(InOutHandle, [this]() {
				TestTrue("Is locked after half time", Env->Lock->IsLocked());
			}, 3.f, false);

			World->GetTimerManager().SetTimer(InOutHandle, [this, DoneDelegate]() {
				TestFalse("Is locked after full time", Env->Lock->IsLocked());
				DoneDelegate.Execute();
			}, 6.01f, false);
		});
	});

	AfterEach([this]()
	{
		Env.Reset();
	});
}

#endif
