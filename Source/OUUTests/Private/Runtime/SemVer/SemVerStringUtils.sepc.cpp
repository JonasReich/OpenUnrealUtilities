// Copyright (c) 2022 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER
	#include "Runtime/SemVer/SemVerTests.h"
	#include "SemVer/SemVerParsingStrictness.h"
	#include "SemVer/SemVerStringUtils.h"

BEGIN_DEFINE_SPEC(FSemVerStringUtilsSpec, "OpenUnrealUtilities.Runtime.SemVer.StringUtils", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FSemVerStringUtilsSpec)
void FSemVerStringUtilsSpec::Define()
{
	Describe("IsValidSemanticVersion", [this]() {
		for (auto Strictness : TEnumRange<ESemVerParsingStrictness>())
		{
			Describe(FString::Printf(TEXT("with Strictness level %s"), *LexToString(Strictness)), [this, Strictness]() {
				for (auto SemVer : ValidSemVers)
				{
					It(EscapeTestName(FString::Printf(TEXT("should return true for semver '%s'"), *SemVer)),
					   [this, Strictness, SemVer]() {
						   SPEC_TEST_TRUE(FSemVerStringUtils::IsValidSemanticVersion(SemVer, Strictness));
					   });
				}
			});
		}

		Describe("with Strictness level Strict", [this]() {
			for (auto SemVer : InvalidSemVers)
			{
				It(EscapeTestName(FString::Printf(TEXT("should return false for semver '%s'"), *SemVer)),
				   [this, SemVer]() {
					   SPEC_TEST_FALSE(
						   FSemVerStringUtils::IsValidSemanticVersion(SemVer, ESemVerParsingStrictness::Strict));
				   });
			}
		});
	});
}

#endif
