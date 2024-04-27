// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Runtime/SemVer/SemVerTests.h"
	#include "SemVer/SemanticVersion.h"

// ReSharper disable StringLiteralTypo

BEGIN_DEFINE_SPEC(FSemanticVersionSpec, "OpenUnrealUtilities.Runtime.SemVer.SemanticVersion", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FSemanticVersionSpec)

void FSemanticVersionSpec::Define()
{
	Describe("Constructing a SemVer", [this]() {
		It("should return 0-1-0 when default initializing", [this]() {
			const FSemanticVersion SemVer;
			SPEC_TEST_EQUAL(SemVer.ToString(), "0.1.0");
		});

		Describe("from three integers", [this]() {
			It("should initialize the major, minor and patch version", [this]() {
				const FSemanticVersion SemVer(42, 69, 404);
				SPEC_TEST_EQUAL(SemVer.ToString(), "42.69.404");
			});

			It("should throw an error when there is a negative number in the arguments and return 0-1-0", [this]() {
				AddExpectedError("No negative numbers allowed");
				const FSemanticVersion SemVer(42, -1, 404);
				SPEC_TEST_EQUAL(SemVer.ToString(), "0.1.0");
			});

			It("should throw an error when all numbers are zero and return 0-1-0", [this]() {
				AddExpectedError("must not all be zero");
				const FSemanticVersion SemVer(0, 0, 0);
				SPEC_TEST_EQUAL(SemVer.ToString(), "0.1.0");
			});
		});

		It("should add a pre-release suffix if it's specified", [this]() {
			const FSemanticVersion SemVer(42, 69, 404, {"alpha"});
			SPEC_TEST_EQUAL(SemVer.ToString(), "42.69.404-alpha");
		});

		It("should add build metadata if it's specified", [this]() {
			const FSemanticVersion SemVer(42, 69, 404, {"alpha"}, {"build1234.6789"});
			SPEC_TEST_EQUAL(SemVer.ToString(), "42.69.404-alpha+build1234.6789");
		});

		It("should initialize the SemVer from a valid SemVer string", [this]() {
			const FString SourceString = "42.69.404-alpha+build1234.6789";
			const FSemanticVersion SemVer(SourceString);
			SPEC_TEST_EQUAL(SemVer.ToString(), SourceString);
		});

		It("should initialize all members correctly so they are available via Get_Version getters", [this]() {
			const FSemanticVersion SemVer(42, 69, 404);
			SPEC_TEST_EQUAL(SemVer.GetMajorVersion(), 42);
			SPEC_TEST_EQUAL(SemVer.GetMinorVersion(), 69);
			SPEC_TEST_EQUAL(SemVer.GetPatchVersion(), 404);
		});
	});

	Describe("IncrementMajorVersion", [this]() {
		It("should increment the major version by one", [this]() {
			FSemanticVersion SemVer(1, 0, 0);
			SemVer.IncrementMajorVersion();
			SPEC_TEST_EQUAL(SemVer.GetMajorVersion(), 2);
		});

		It("should reset the minor and patch version to 0", [this]() {
			FSemanticVersion SemVer(42, 69, 404);
			SemVer.IncrementMajorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{43, 0, 0}));
		});

		It("should strip pre-release identifier and build metadata", [this]() {
			FSemanticVersion SemVer(42, 69, 404, {"alpha"}, {"build1234.6789"});
			SemVer.IncrementMajorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{43, 0, 0}));
		});
	});

	Describe("IncrementMinorVersion", [this]() {
		It("should increment the minor version by one", [this]() {
			FSemanticVersion SemVer(1, 1, 0);
			SemVer.IncrementMinorVersion();
			SPEC_TEST_EQUAL(SemVer.GetMinorVersion(), 2);
		});

		It("should reset the patch version to 0", [this]() {
			FSemanticVersion SemVer(42, 69, 404);
			SemVer.IncrementMinorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{42, 70, 0}));
		});

		It("should strip pre-release identifier and build metadata", [this]() {
			FSemanticVersion SemVer(42, 69, 404, {"alpha"}, {"build1234.6789"});
			SemVer.IncrementMinorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{42, 70, 0}));
		});
	});

	Describe("IncrementPatchVersion", [this]() {
		It("should increment the patch version by one", [this]() {
			FSemanticVersion SemVer(1, 0, 1);
			SemVer.IncrementPatchVersion();
			SPEC_TEST_EQUAL(SemVer.GetPatchVersion(), 2);
		});

		It("should strip pre-release identifier and build metadata", [this]() {
			FSemanticVersion SemVer(42, 69, 404, {"alpha"}, {"build1234.6789"});
			SemVer.IncrementPatchVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{42, 69, 405}));
		});
	});

	Describe("TryIncrementPrereleaseVersion", [this]() {
		It("should increase the pre-release version if it's a simple number", [this]() {
			FSemanticVersion SemVer(1, 0, 0, {"4"});
			const bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			SPEC_TEST_TRUE(bSuccess);
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{1, 0, 0, {"5"}}));
		});

		It("should fail to increase the pre-release version if it's a string", [this]() {
			FSemanticVersion SemVer(1, 0, 0, {"alpha"});
			const bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			SPEC_TEST_FALSE(bSuccess);
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{1, 0, 0, {"alpha"}}));
		});

		It("should increase the pre-release version if the last identifier in it is a number", [this]() {
			FSemanticVersion SemVer(1, 0, 0, {"alpha.2"});
			const bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			SPEC_TEST_TRUE(bSuccess);
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{1, 0, 0, {"alpha.3"}}));
		});

		It("should fail to increase the pre-release version if the second-to-last identifier is a number but the last "
		   "identifier is a string",
		   [this]() {
			   FSemanticVersion SemVer(1, 0, 0, {"alpha.2.x"});
			   const bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			   SPEC_TEST_FALSE(bSuccess);
			   SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{1, 0, 0, {"alpha.2.x"}}));
		   });
	});

	Describe("ToString", [this]() {
		for (auto SemVerString : ValidSemVers)
		{
			It(EscapeTestName(FString::Printf(
				   TEXT("should return the semver string that was used to construct it (%s)"),
				   *SemVerString)),
			   [this, SemVerString]() {
				   const FSemanticVersion SemVer(SemVerString);
				   const FString ResultString = SemVer.ToString();
				   SPEC_TEST_EQUAL(ResultString, SemVerString);
			   });
		}
	});

	Describe("TryParseString", [this]() {
		for (auto Strictness : TEnumRange<ESemVerParsingStrictness>())
		{
			Describe(FString::Printf(TEXT("with Strictness level %s"), *LexToString(Strictness)), [this, Strictness]() {
				for (auto SemVerString : ValidSemVers)
				{
					Describe("should succeed on spec complient semver", [this, Strictness, SemVerString]() {
						It(EscapeTestName(FString::Printf(TEXT("%s"), *SemVerString)),
						   [this, Strictness, SemVerString]() {
							   FSemanticVersion SemVer;
							   const bool bResult = SemVer.TryParseString(SemVerString, Strictness);
							   const FString ResultString = SemVer.ToString();
							   SPEC_TEST_TRUE(bResult);
							   SPEC_TEST_EQUAL(ResultString, SemVerString);
						   });
					});
				}
			});
		}

		Describe("with Strictness level Strict", [this]() {
			Describe("should fail on spec non-compliant semver", [this]() {
				for (auto SemVerString : InvalidSemVers)
				{
					It(EscapeTestName(FString::Printf(TEXT("%s"), *SemVerString)), [this, SemVerString]() {
						FSemanticVersion SemVer;
						const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Strict);
						FString ResultString = SemVer.ToString();
						SPEC_TEST_FALSE(bResult);
					});
				}
			});
		});

		Describe("with Strictness level Regular", [this]() {
			It("should succeed with semvers that have leading zeroes", [this]() {
				const FString SemVerString = "01.006.010-01.0build.02";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.6.10-1.0build.2");
			});

			It("should succeed with semvers that have special characters in the build metadata", [this]() {
				const FString SemVerString = "1.0.5+build@meta#data";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.0.5+build@meta#data");
			});

			It("should fail with semvers that have less than three digits", [this]() {
				const FString SemVerString = "4.0";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_FALSE(bResult);
			});

			It("should fail with semvers that have more than three digits", [this]() {
				const FString SemVerString = "1.2.3.4";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_FALSE(bResult);
			});

			It("should fail with semvers that contain whitespace", [this]() {
				const FString SemVerString = "1.2.3.4 ";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_FALSE(bResult);
			});
		});

		Describe("with Strictness level Liberal", [this]() {
			It("should succeed with semvers that have leading zeroes", [this]() {
				const FString SemVerString = "01.006.010-01.0build.02";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.6.10-1.0build.2");
			});

			It("should succeed with semvers that have special characters in the build metadata", [this]() {
				const FString SemVerString = "1.0.5+build@meta#data";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.0.5+build@meta#data");
			});

			It("should succeed with semvers that have a single digits", [this]() {
				const FString SemVerString = "42";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "42.0.0");
			});

			It("should succeed with semvers that have two digits", [this]() {
				const FString SemVerString = "4.3";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "4.3.0");
			});

			It("should succeed with semvers that have more than three digits", [this]() {
				const FString SemVerString = "1.2.3.4";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3-4");
			});

			It("should succeed with semvers that contain whitespace", [this]() {
				const FString SemVerString = "1.2.3 -build-metadata";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3");
			});

			It("should succeed with semvers that have arbitrary prefixes", [this]() {
				const FString SemVerString = "Version#=1.2.3-alpha+build";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3-alpha+build");
			});

			It("should succeed with semvers that have arbitrary prefixes or suffixes", [this]() {
				const FString SemVerString = "The version 1.2.3-alpha+build is the version we need";
				FSemanticVersion SemVer;
				const bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				const FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3-alpha+build");
			});
		});

		Describe("EqualsPrecedence", [this]() {
			It("should return false for semvers with different build numbers", [this]() {
				const FSemanticVersion A("1.2.3");
				const FSemanticVersion B("2.3.4");
				SPEC_TEST_FALSE(A.EqualsPrecedence(B));
				SPEC_TEST_FALSE(B.EqualsPrecedence(A));
			});

			It("should return false for semvers with different pre-release identifier", [this]() {
				const FSemanticVersion A("1.2.3-alpha");
				const FSemanticVersion B("1.2.3-beta");
				SPEC_TEST_FALSE(A.EqualsPrecedence(B));
				SPEC_TEST_FALSE(B.EqualsPrecedence(A));
			});

			It("should return true for matching semvers", [this]() {
				const FSemanticVersion A("1.2.3-alpha+build2");
				const FSemanticVersion B("1.2.3-alpha+build2");
				SPEC_TEST_TRUE(A.EqualsPrecedence(B));
				SPEC_TEST_TRUE(B.EqualsPrecedence(A));
			});

			It("should ignore differences in build number", [this]() {
				const FSemanticVersion A("1.2.3-alpha+build2");
				const FSemanticVersion B("1.2.3-alpha+build3");
				SPEC_TEST_TRUE(A.EqualsPrecedence(B));
				SPEC_TEST_TRUE(B.EqualsPrecedence(A));
			});
		});
	});

	Describe("comparison operators", [this]() {
		Describe("==", [this]() {
			It("should return false for semvers with different build numbers", [this]() {
				const FSemanticVersion A("1.2.3");
				const FSemanticVersion B("2.3.4");
				SPEC_TEST_FALSE(A == B);
				SPEC_TEST_FALSE(B == A);
			});

			It("should return false for semvers with different pre-release identifier", [this]() {
				const FSemanticVersion A("1.2.3-alpha");
				const FSemanticVersion B("1.2.3-beta");
				SPEC_TEST_FALSE(A == B);
				SPEC_TEST_FALSE(B == A);
			});

			It("should return true for matching semvers", [this]() {
				const FSemanticVersion A("1.2.3-alpha+build2");
				const FSemanticVersion B("1.2.3-alpha+build2");
				SPEC_TEST_TRUE(A == B);
				SPEC_TEST_TRUE(B == A);
			});

			It("should NOT ignore differences in build number", [this]() {
				const FSemanticVersion A("1.2.3-alpha+build2");
				const FSemanticVersion B("1.2.3-alpha+build3");
				SPEC_TEST_FALSE(A == B);
				SPEC_TEST_FALSE(B == A);
			});
		});

		Describe("precedence comparison operators", [this]() {
			const TArray<FSemanticVersion> InSemVers{
				{"2.0.0"},
				{"1.3.0-alpha"},
				{"1.3.0"},
				{"1.2.0-beta"},
				{"1.2.0-alpha"},
			};

			const TArray<FSemanticVersion>
				ExpectedSorting{{"1.2.0-alpha"}, {"1.2.0-beta"}, {"1.3.0-alpha"}, {"1.3.0"}, {"2.0.0"}};

			It("operator<.should be usable to sort semvers by precedence", [this, InSemVers, ExpectedSorting]() {
				TArray<FSemanticVersion> SemVerWorkingCopy = InSemVers;
				SemVerWorkingCopy.Sort([](auto& A, auto& B) -> bool { return A < B; });
				TestArraysEqual(*this, "semvers sorted by precedence", SemVerWorkingCopy, ExpectedSorting, true);
			});

			It("operator>.should be usable to sort semvers by precedence (reverse order)",
			   [this, InSemVers, ExpectedSorting]() {
				   TArray<FSemanticVersion> SemVerWorkingCopy = InSemVers;
				   SemVerWorkingCopy.Sort([](auto& A, auto& B) -> bool { return A > B; });
				   TArray<FSemanticVersion> ReverseExpectedSorting = ExpectedSorting;
				   Algo::Reverse(ReverseExpectedSorting);
				   TestArraysEqual(
					   *this,
					   "semvers sorted by precedence (>)",
					   SemVerWorkingCopy,
					   ReverseExpectedSorting,
					   true);
			   });

			for (int32 i = 0; i < ExpectedSorting.Num(); i++)
			{
				for (int32 j = 0; j < ExpectedSorting.Num(); j++)
				{
					auto A = ExpectedSorting[i];
					auto B = ExpectedSorting[j];
					if (i < j)
					{
						It("operator<."
							   + EscapeTestName(
								   FString::Printf(TEXT("%s should be smaller than %s"), *A.ToString(), *B.ToString())),
						   [this, A, B]() {
							   TestTrue(FString::Printf(TEXT("%s < %s"), *A.ToString(), *B.ToString()), A < B);
						   });
					}
					else
					{
						It("operator<."
							   + EscapeTestName(FString::Printf(
								   TEXT("%s should NOT be smaller than %s"),
								   *A.ToString(),
								   *B.ToString())),
						   [this, A, B]() {
							   TestFalse(FString::Printf(TEXT("%s < %s"), *A.ToString(), *B.ToString()), A < B);
						   });
					}

					if (i > j)
					{
						It("operator>."
							   + EscapeTestName(
								   FString::Printf(TEXT("%s should be greater than %s"), *A.ToString(), *B.ToString())),
						   [this, A, B]() {
							   TestTrue(FString::Printf(TEXT("%s > %s"), *A.ToString(), *B.ToString()), A > B);
						   });
					}
					else
					{
						It("operator>."
							   + EscapeTestName(FString::Printf(
								   TEXT("%s should NOT be greater than %s"),
								   *A.ToString(),
								   *B.ToString())),
						   [this, A, B]() {
							   TestFalse(FString::Printf(TEXT("%s > %s"), *A.ToString(), *B.ToString()), A > B);
						   });
					}
				}
			}
		});
	});
}

#endif
