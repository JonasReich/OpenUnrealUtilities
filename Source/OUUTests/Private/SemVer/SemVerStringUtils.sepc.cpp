// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER
#include "SemVer/SemVerStringUtils.h"
#include "SemVer/SemVerTests.h"
#include "SemVer/SemVerParsingStrictness.h"

BEGIN_DEFINE_SPEC(FSemVerStringUtilsSpec, "OpenUnrealUtilities.SemVer.SemVerStringUtils", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FSemVerStringUtilsSpec)
void FSemVerStringUtilsSpec::Define()
{
	Describe("IsValidSemanticVersion", [this]()
	{
		for (auto Strictness : TEnumRange<ESemVerParsingStrictness>())
		{
			Describe(FString::Printf(TEXT("with Strictness level %s"), *LexToString(Strictness)), [this, Strictness]()
			{
				for (auto SemVer : ValidSemVers)
				{
					It(EscapeTestName(FString::Printf(TEXT("should return true for semver '%s'"), *SemVer)), [this, Strictness, SemVer]()
					{
						SPEC_TEST_TRUE(FSemVerStringUtils::IsValidSemanticVersion(SemVer, Strictness));
					});
				}


			});
		}

		Describe("with Strictness level Strict", [this]()
		{
			for (auto SemVer : InvalidSemVers)
			{
				It(EscapeTestName(FString::Printf(TEXT("should return false for semver '%s'"), *SemVer)), [this, SemVer]()
				{
					SPEC_TEST_FALSE(FSemVerStringUtils::IsValidSemanticVersion(SemVer, ESemVerParsingStrictness::Strict));
				});
			}
		});
	});
}

#endif
