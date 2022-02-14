// Copyright (c) 2022 Jonas Reich

#include "OUUTestUtilities.h"
#include "Templates/ScopedMultiRWLock.h"

#if WITH_AUTOMATION_WORKER

	#include "Templates/RWLockedVariable.h"

BEGIN_DEFINE_SPEC(
	FRWLockedVariableSpec,
	"OpenUnrealUtilities.Runtime.Templates.RWLockedVariable",
	DEFAULT_OUU_TEST_FLAGS)
	TRWLockedVariable<int32> RWLockedInt{INDEX_NONE};
END_DEFINE_SPEC(FRWLockedVariableSpec)

void FRWLockedVariableSpec::Define()
{
	BeforeEach([this]() {
		int32& IntRef = RWLockedInt.Variable;
		IntRef = 2;
	});

	Describe("Read", [this]() {
		It("should return the last value the variable was set to",
		   [this]() { SPEC_TEST_EQUAL(RWLockedInt.Read().Get(), 2); });
	});

	Describe("Write", [this]() {
		It("should allow changing the variable to something else", [this]() {
			RWLockedInt.Write() = 8;
			SPEC_TEST_EQUAL(RWLockedInt.Read().Get(), 8);
		});
	});
}

#endif
