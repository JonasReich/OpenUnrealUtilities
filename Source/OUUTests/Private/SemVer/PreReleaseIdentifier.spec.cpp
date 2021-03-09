// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "SemVer/PreReleaseIdentifier.h"

BEGIN_DEFINE_SPEC(FSemVerPreReleaseIdentifierSpec, "OpenUnrealUtilities.SemVer.PreReleaseIdentifier", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FSemVerPreReleaseIdentifierSpec)

void FSemVerPreReleaseIdentifierSpec::Define()
{
	// construction + string parsing is indirectly tested via SemanticVersion spec

	Describe("TryIncrement", [this]()
	{
		It("should fail if the identifier has no number arguments", [this]()
		{
			FSemVerPreReleaseIdentifier PreRelease("beta");
			SPEC_TEST_FALSE(PreRelease.TryIncrement());
			SPEC_TEST_EQUAL(PreRelease, {"beta"});
		});

		It("should fail if the identifier has a number but ends in a letter", [this]()
		{
			FSemVerPreReleaseIdentifier PreRelease("beta.5.a");
			SPEC_TEST_FALSE(PreRelease.TryIncrement());
			SPEC_TEST_EQUAL(PreRelease, {"beta.5.a"});
		});

		It("should fail if the identifier is a mixture of numbers and letters", [this]()
		{
			FSemVerPreReleaseIdentifier PreRelease("45b");
			SPEC_TEST_FALSE(PreRelease.TryIncrement());
			SPEC_TEST_EQUAL(PreRelease, {"45b"});
		});

		It("should succeed if the identifier is a single number", [this]()
		{
			FSemVerPreReleaseIdentifier PreRelease("123");
			SPEC_TEST_TRUE(PreRelease.TryIncrement());
			SPEC_TEST_EQUAL(PreRelease, {"124"});
		});

		It("should succeed if the last identifier is a number", [this]()
		{
			FSemVerPreReleaseIdentifier PreRelease("alpha.2.a.123");
			SPEC_TEST_TRUE(PreRelease.TryIncrement());
			SPEC_TEST_EQUAL(PreRelease, {"alpha.2.a.124"});
		});
	});

	Describe("equality operator ==", [this]()
	{
		It("should return true if two semvers are the same", [this]()
		{
			FSemVerPreReleaseIdentifier A("1.2.alpha");
			FSemVerPreReleaseIdentifier B("1.2.alpha");
			SPEC_TEST_TRUE(A == B);
		});

		It("should return false if two semvers are not the same (1)", [this]()
		{
			FSemVerPreReleaseIdentifier A("1.2.alpha");
			FSemVerPreReleaseIdentifier B("1.3.alpha");
			SPEC_TEST_FALSE(A == B);
		});

		It("should return false if two semvers are not the same (2)", [this]()
		{
			FSemVerPreReleaseIdentifier A("1.2.alpha");
			FSemVerPreReleaseIdentifier B("1.2.beta");
			SPEC_TEST_FALSE(A == B);
		});
	});

	Describe("precedence comparison operators", [this]()
	{
		const TArray<FSemVerPreReleaseIdentifier> PreReleases = {
			{"1.3.3.1"},
			{"1.2.3.a"},
			{"beta.1"},
			{"alpha.1"},
			{"1.2.3.b"},
		};

		const TArray<FSemVerPreReleaseIdentifier> ExpectedSortOrder = {
			{"1.2.3.a"},
			{"1.2.3.b"},
			{"1.3.3.1"},
			{"alpha.1"},
			{"beta.1"},
		};

		It("operator<.should be usable to sort pre-releases by precedence", [this, PreReleases, ExpectedSortOrder]()
		{
			TArray<FSemVerPreReleaseIdentifier> PreReleasesWorkCopy = PreReleases;
			PreReleasesWorkCopy.Sort([](auto& A, auto& B) -> bool { return A < B; });
			TestArraysEqual(*this, "pre-releases sorted by precedence", PreReleasesWorkCopy, ExpectedSortOrder, true);
		});

		It("operator>.should be usable to sort pre-releases by precedence (reverse order)", [this, PreReleases, ExpectedSortOrder]()
		{
			TArray<FSemVerPreReleaseIdentifier> PreReleasesWorkCopy = PreReleases;
			PreReleasesWorkCopy.Sort([](auto& A, auto& B) -> bool { return A > B; });
			TArray<FSemVerPreReleaseIdentifier> ReverseExpectedSorting = ExpectedSortOrder;
			Algo::Reverse(ReverseExpectedSorting);
			TestArraysEqual(*this, "pre-releases sorted by precedence (>)", PreReleasesWorkCopy, ReverseExpectedSorting, true);
		});

		for (int32 i = 0; i < ExpectedSortOrder.Num(); i++)
		{
			for (int32 j = 0; j < ExpectedSortOrder.Num(); j++)
			{
				auto A = ExpectedSortOrder[i];
				auto B = ExpectedSortOrder[j];
				if (i < j)
				{
					It("operator<." + EscapeTestName(FString::Printf(TEXT("%s should be smaller than %s"), *A.ToString(), *B.ToString())), [this, A, B]()
					{
						TestTrue(FString::Printf(TEXT("%s < %s"), *A.ToString(), *B.ToString()), A < B);
					});
				}
				else
				{
					It("operator<." + EscapeTestName(FString::Printf(TEXT("%s should NOT be smaller than %s"), *A.ToString(), *B.ToString())), [this, A, B]()
					{
						TestFalse(FString::Printf(TEXT("%s < %s"), *A.ToString(), *B.ToString()), A < B);
					});
				}

				if (i > j)
				{
					It("operator>." + EscapeTestName(FString::Printf(TEXT("%s should be greater than %s"), *A.ToString(), *B.ToString())), [this, A, B]()
					{
						TestTrue(FString::Printf(TEXT("%s > %s"), *A.ToString(), *B.ToString()), A > B);
					});
				}
				else
				{
					It("operator>." + EscapeTestName(FString::Printf(TEXT("%s should NOT be greater than %s"), *A.ToString(), *B.ToString())), [this, A, B]()
					{
						TestFalse(FString::Printf(TEXT("%s > %s"), *A.ToString(), *B.ToString()), A > B);
					});
				}
			}
		}
	});
}

#endif
