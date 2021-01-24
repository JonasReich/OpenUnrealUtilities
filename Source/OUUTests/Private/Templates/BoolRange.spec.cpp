// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/BoolRange.h"
#include "Templates/ReverseIterator.h"

BEGIN_DEFINE_SPEC(FBoolRangeSpec, "OpenUnrealUtilities.Templates.BoolRange", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FBoolRangeSpec)
void FBoolRangeSpec::Define()
{
	Describe("", [this]()
	{
		It("should be a single pair of {false, true}", [this]()
		{
			TArray<bool> Booleans;
			for (bool Boolean : BoolRange())
			{
				Booleans.Add(Boolean);
			}
			TestEqual("Booleans", Booleans, { false, true });
		});
		It("should be a single pair of {true, false} when reversed", [this]() 
		{
			TArray<bool> Booleans;
			for (bool Boolean : ReverseRange(BoolRange()))
			{
				Booleans.Add(Boolean);
			}
			TestEqual("Booleans", Booleans, { true, false });
		});
	});
}

#endif
