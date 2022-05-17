// Copyright (c) 2022 Jonas Reich

#include "Templates/ScopedAssign.h"

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Templates/RWLockedVariable.h"

BEGIN_DEFINE_SPEC(FScopedAssignSpec, "OpenUnrealUtilities.Runtime.Templates.ScopedAssign", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FScopedAssignSpec)

void FScopedAssignSpec::Define()
{
	It("should reset the value to it's original after it runs out of scope", [this]() {
		int32 i = 0;
		SPEC_TEST_EQUAL(i, 0);
		{
			TScopedAssign ScopedAssign{i, 42};
			SPEC_TEST_EQUAL(i, 42);
		}
		SPEC_TEST_EQUAL(i, 0);
	});
}

#endif
