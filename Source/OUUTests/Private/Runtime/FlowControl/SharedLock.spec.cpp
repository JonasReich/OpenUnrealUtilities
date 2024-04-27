// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"
#include "TimerManager.h"

#if WITH_AUTOMATION_WORKER

	#include "Runtime/FlowControl/LockTestHelpers.h"

BEGIN_DEFINE_SPEC(FSharedLockSpec, "OpenUnrealUtilities.Runtime.FlowControl.SharedLock", DEFAULT_OUU_TEST_FLAGS)
	TSharedPtr<FOUULockTestEnvironment<USharedLock>> Env;
END_DEFINE_SPEC(FSharedLockSpec)
void FSharedLockSpec::Define()
{
	BeforeEach([this]() { Env = MakeShared<FOUULockTestEnvironment<USharedLock>>(*this); });

	Describe("Lock", [this]() {
		It("should fail to lock the lock if the key object is nullptr", [this]() {
			Env->Lock->Lock(nullptr);
			SPEC_TEST_FALSE(Env->Lock->IsLocked());
		});

		It("should succeed to lock the lock if the key object is a valid object", [this]() {
			Env->Lock->Lock(Env->Key0);
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});

		It("should succeed without error when called twice", [this]() {
			Env->Lock->Lock(Env->Key0);
			Env->Lock->Lock(Env->Key0);
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});

		It("should succeed to lock the lock with a second key after it was already locked by a different object",
		   [this]() {
			   Env->Lock->Lock(Env->Key0);
			   Env->Lock->Lock(Env->Key1);
			   SPEC_TEST_TRUE(Env->Lock->IsLocked());
		   });
	});

	Describe("TryUnlock", [this]() {
		It("should succeed if the lock was not locked before",
		   [this]() { SPEC_TEST_TRUE(Env->Lock->TryUnlock(Env->Key0)); });

		It("should succeed if the lock was locked with the same key object", [this]() {
			Env->Lock->Lock(Env->Key0);
			SPEC_TEST_TRUE(Env->Lock->TryUnlock(Env->Key0));
			SPEC_TEST_FALSE(Env->Lock->IsLocked());
		});

		It("should fail if the lock was locked with a different object", [this]() {
			Env->Lock->Lock(Env->Key0);
			SPEC_TEST_FALSE(Env->Lock->TryUnlock(Env->Key1));
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});

		It("should fail if the lock was also locked with a second object", [this]() {
			Env->Lock->Lock(Env->Key0);
			Env->Lock->Lock(Env->Key1);
			SPEC_TEST_FALSE(Env->Lock->TryUnlock(Env->Key0));
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
		});

		It("should succeed if the lock is unlocked with all key objects that were used to lock it before", [this]() {
			Env->Lock->Lock(Env->Key0);
			Env->Lock->Lock(Env->Key1);
			Env->Lock->TryUnlock(Env->Key0);
			SPEC_TEST_TRUE(Env->Lock->TryUnlock(Env->Key1));
			SPEC_TEST_FALSE(Env->Lock->IsLocked());
		});

		It("should succeed if the lock is unlocked with all key objects that were used to lock it before - in opposite "
		   "order",
		   [this]() {
			   Env->Lock->Lock(Env->Key0);
			   Env->Lock->Lock(Env->Key1);
			   Env->Lock->TryUnlock(Env->Key1);
			   SPEC_TEST_TRUE(Env->Lock->TryUnlock(Env->Key0));
			   SPEC_TEST_FALSE(Env->Lock->IsLocked());
		   });

		It("should make the lock lockable with the same key object again", [this]() {
			Env->Lock->Lock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			Env->Lock->Lock(Env->Key0);
			SPEC_TEST_TRUE(Env->Lock->IsLocked());
			Env->Lock->TryUnlock(Env->Key0);
			SPEC_TEST_FALSE(Env->Lock->IsLocked());
		});
	});

	Describe("OnLockStateChanged", [this]() {
		It("should be called when the lock is locked", [this]() {
			Env->Lock->Lock(Env->Key0);
			SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 1);
		});

		It("should be called again when the lock is unlocked", [this]() {
			Env->Lock->Lock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 2);
		});

		It("should not be called again when the lock state doesn't change (for example when the lock is locked twice "
		   "in a row)",
		   [this]() {
			   Env->Lock->Lock(Env->Key0);
			   Env->Lock->Lock(Env->Key1);
			   SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 1);
		   });

		It("should pass the correct lock and lock state as parameters", [this]() {
			Env->Lock->Lock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			Env->Lock->Lock(Env->Key1);
			Env->Lock->Lock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key0);
			Env->Lock->TryUnlock(Env->Key1);

			SPEC_TEST_TRUE(Env->Helper->bLockAlwaysSame);
			SPEC_TEST_EQUAL(Env->Helper->Lock, Cast<UObject>(Env->Lock));

			SPEC_TEST_EQUAL(Env->Helper->NumDelegateCalls, 4);
			const TArray<bool> Booleans{true, false, true, false};
			SPEC_TEST_ARRAYS_EQUAL(Env->Helper->LockStateHistory, Booleans);
		});
	});

	Describe("LockForDuration", [this]() {
		It("should lock the lock for specified duration", [this]() {
			Env->Lock->LockForDuration(Env->Key0, 6.f);

			TestTrue("Is locked at beginning", Env->Lock->IsLocked());

			FTimerHandle InOutHandle;
			Env->TestWorld.World->GetTimerManager().SetTimer(
				InOutHandle,
				[this]() { TestTrue("Is locked after half time", Env->Lock->IsLocked()); },
				3.f,
				false);

			Env->TestWorld.World->GetTimerManager().SetTimer(
				InOutHandle,
				[this]() { TestFalse("Is locked after full time", Env->Lock->IsLocked()); },
				6.01f,
				false);

			Env->TestWorld.World->GetTimerManager().Tick(3.f);
			Env->TestWorld.World->GetTimerManager().Tick(3.f);
			Env->TestWorld.World->GetTimerManager().Tick(3.f);
		});
	});

	AfterEach([this]() { Env.Reset(); });
}

#endif
