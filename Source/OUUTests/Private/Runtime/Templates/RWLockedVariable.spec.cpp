// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/RWLockedVariable.h"

BEGIN_DEFINE_SPEC(FRWLockedVariableSpec, "OpenUnrealUtilities.Runtime.Templates.RWLockedVariable", DEFAULT_OUU_TEST_FLAGS)
TRWLockedVariable<int32> RWLockedFloat;
END_DEFINE_SPEC(FRWLockedVariableSpec)

void FRWLockedVariableSpec::Define()
{
	BeforeEach([this]()
	{
		auto FloatRef = RWLockedFloat.Write();
		FloatRef = 10;
	});

	Describe("Read", [this]()
	{
		auto FloatRef = RWLockedFloat.Read();
		//FloatRef = 5;
		SPEC_TEST_EQUAL(*FloatRef, 5);
	});
}

#endif
