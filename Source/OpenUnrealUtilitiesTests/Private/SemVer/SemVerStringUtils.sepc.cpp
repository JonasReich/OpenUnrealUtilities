// Copyright (c) 2020 Jonas Reich

#include "OUUTests.h"
#include "SemVer/SemVerStringUtils.h"
#include "SemVer/SemVerTests.h"

#if WITH_AUTOMATION_WORKER

BEGIN_DEFINE_SPEC(FSemVarStringUtilsSpec, "OpenUnrealUtilities.SemVer.SemVerStringUtils", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FSemVarStringUtilsSpec)
void FSemVarStringUtilsSpec::Define()
{
	Describe("IsValidSemanticVersion", [this]()
	{
		for (auto SemVer : ValidSemVers)
		{
			It(EscapeTestName(FString::Printf(TEXT("should return true for semver '%s'"), *SemVer)), [this, SemVer]()
			{
				SPEC_TEST_TRUE(FSemVerStringUtils::IsValidSemanticVersion(SemVer));
			});
		}

		for (auto SemVer : InvalidSemVers)
		{
			It(EscapeTestName(FString::Printf(TEXT("should return false for semver '%s'"), *SemVer)), [this, SemVer]()
			{
				SPEC_TEST_FALSE(FSemVerStringUtils::IsValidSemanticVersion(SemVer));
			});
		}
	});
}

#endif
