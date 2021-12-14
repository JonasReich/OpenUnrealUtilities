// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"
#include "Templates/ScopedMultiRWLock.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/RWLockedVariable.h"

BEGIN_DEFINE_SPEC(FScopedMultiRWLockSpec, "OpenUnrealUtilities.Runtime.Templates.ScopedMultiRWLock", DEFAULT_OUU_TEST_FLAGS)
TRWLockedVariable<int32> RWLockedInt;
TRWLockedVariable<TArray<int32>> RWLockedArray;
END_DEFINE_SPEC(FScopedMultiRWLockSpec)

void FScopedMultiRWLockSpec::Define()
{
	BeforeEach([this]()
	{
		auto IntRef = RWLockedInt.Write();
		IntRef = 5;
	});

	Describe("GetPointers", [this]()
	{
		It ("should be tied to pointers that allow accessing all values", [this]()
		{
			int32& RealInt = RWLockedInt.GetRefWithoutLocking_USE_WITH_CAUTION();
			RealInt = 42;
			SPEC_TEST_EQUAL(RealInt, 42);

			const auto ScopedMultiLock = MakeScopedMultiRWLock(Read(RWLockedArray), Write(RWLockedInt));

			// -- retrieve by Tie = static
			const TArray<int32>* ArrayPtr = nullptr;
			int32* IntPtr = nullptr;

			// This would not compile because of missing const!
			// TArray<int32>* ArrayPtr = nullptr;

			Tie(ArrayPtr, IntPtr) = ScopedMultiLock.GetPointers();
			*IntPtr = 15;
			SPEC_TEST_EQUAL(RealInt, 15);
		});
	});

	Describe("GetByIdx", [this]()
	{
		It ("should give access to a single value via reference", [this]()
		{
			int32& RealInt = RWLockedInt.GetRefWithoutLocking_USE_WITH_CAUTION();
			RealInt = 42;
			SPEC_TEST_EQUAL(RealInt, 42);

			const auto ScopedMultiLock = MakeScopedMultiRWLock(Read(RWLockedArray), Write(RWLockedInt));

			// -- retrieve by index = static
			int32& IntRef1 = ScopedMultiLock.GetByIdx<1>();
			IntRef1 = 3;
			SPEC_TEST_EQUAL(RealInt, 3);
		});
	});
}

#endif
