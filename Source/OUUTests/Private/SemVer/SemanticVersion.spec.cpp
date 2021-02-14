// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "SemVer/SemanticVersion.h"
#include "SemVer/SemVerTests.h"

BEGIN_DEFINE_SPEC(FSemanticVersionSpec, "OpenUnrealUtilities.SemVer.SemanticVersion", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FSemanticVersionSpec)
void FSemanticVersionSpec::Define()
{
	Describe("Constructing a SemVer", [this]()
	{
		It("should return 0-1-0 when default initializing", [this]()
		{
			FSemanticVersion SemVer;
			SPEC_TEST_EQUAL(SemVer.ToString(), "0.1.0");
		});

		Describe("from three integers", [this]()
		{
			It("should initialize the major, minor and patch version", [this]()
			{
				FSemanticVersion SemVer(42, 69, 404);
				SPEC_TEST_EQUAL(SemVer.ToString(), "42.69.404");
			});

			It("should throw an error when there is a negative number in the arguments and return 0-1-0", [this]()
			{
				AddExpectedError("No negative numbers allowed");
				FSemanticVersion SemVer(42, -1, 404);
				SPEC_TEST_EQUAL(SemVer.ToString(), "0.1.0");
			});

			It("should throw an error when all numbers are zero and return 0-1-0", [this]()
			{
				AddExpectedError("must not all be zero");
				FSemanticVersion SemVer(0, 0, 0);
				SPEC_TEST_EQUAL(SemVer.ToString(), "0.1.0");
			});
		});

		It("should add a pre-release suffix if it's specified", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404, { "alpha" });
			SPEC_TEST_EQUAL(SemVer.ToString(), "42.69.404-alpha");
		});

		It("should add build metadata if it's specified", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404, { "alpha" }, { "build1234.6789" });
			SPEC_TEST_EQUAL(SemVer.ToString(), "42.69.404-alpha+build1234.6789");
		});

		It("should initialize the SemVer from a valid SemVer string", [this]()
		{
			FString SourceString = "42.69.404-alpha+build1234.6789";
			FSemanticVersion SemVer(SourceString);
			SPEC_TEST_EQUAL(SemVer.ToString(), SourceString);
		});

		It("should initialize all members correctly so they are available via Get_Version getters", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404);
			SPEC_TEST_EQUAL(SemVer.GetMajorVersion(), 42);
			SPEC_TEST_EQUAL(SemVer.GetMinorVersion(), 69);
			SPEC_TEST_EQUAL(SemVer.GetPatchVersion(), 404);
		});
	});

	Describe("IncrementMajorVersion", [this]()
	{
		It("should increment the major version by one", [this]()
		{
			FSemanticVersion SemVer(1, 0, 0);
			SemVer.IncrementMajorVersion();
			SPEC_TEST_EQUAL(SemVer.GetMajorVersion(), 2);
		});

		It("should reset the minor and patch version to 0", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404);
			SemVer.IncrementMajorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{43, 0, 0}));
		});

		It("should strip pre-release identifier and build metadata", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404, { "alpha" }, { "build1234.6789" });
			SemVer.IncrementMajorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 43, 0, 0 }));
		});
	});

	Describe("IncrementMinorVersion", [this]()
	{
		It("should increment the minor version by one", [this]()
		{
			FSemanticVersion SemVer(1, 1, 0);
			SemVer.IncrementMinorVersion();
			SPEC_TEST_EQUAL(SemVer.GetMinorVersion(), 2);
		});

		It("should reset the patch version to 0", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404);
			SemVer.IncrementMinorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 42, 70, 0 }));
		});

		It("should strip pre-release identifier and build metadata", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404, { "alpha" }, { "build1234.6789" });
			SemVer.IncrementMinorVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 42, 70, 0 }));
		});
	});

	Describe("IncrementPatchVersion", [this]()
	{
		It("should increment the patch version by one", [this]()
		{
			FSemanticVersion SemVer(1, 0, 1);
			SemVer.IncrementPatchVersion();
			SPEC_TEST_EQUAL(SemVer.GetPatchVersion(), 2);
		});

		It("should strip pre-release identifier and build metadata", [this]()
		{
			FSemanticVersion SemVer(42, 69, 404, { "alpha" }, { "build1234.6789" });
			SemVer.IncrementPatchVersion();
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 42, 69, 405 }));
		});
	});

	Describe("TryIncrementPrereleaseVersion", [this]()
	{
		It("should increase the pre-release version if it's a simple number", [this]()
		{
			FSemanticVersion SemVer(1, 0, 0, { "4" });
			bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			SPEC_TEST_TRUE(bSuccess);
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 1, 0, 0, { "5" } }));
		});

		It("should fail to increase the pre-release version if it's a string", [this]()
		{
			FSemanticVersion SemVer(1, 0, 0, { "alpha" });
			bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			SPEC_TEST_FALSE(bSuccess);
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 1, 0, 0, { "alpha" } }));
		});

		It("should increase the pre-release version if the last identifier in it is a number", [this]()
		{
			FSemanticVersion SemVer(1, 0, 0, { "alpha.2" });
			bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			SPEC_TEST_TRUE(bSuccess);
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 1, 0, 0, { "alpha.3" } }));
		});

		It("should fail to increase the pre-release version if the second-to-last identifier is a number but the last identifier is a string", [this]()
		{
			FSemanticVersion SemVer(1, 0, 0, { "alpha.2.x" });
			bool bSuccess = SemVer.TryIncrementPreReleaseVersion();
			SPEC_TEST_FALSE(bSuccess);
			SPEC_TEST_EQUAL(SemVer, (FSemanticVersion{ 1, 0, 0, { "alpha.2.x" } }));
		});
	});

	Describe("ToString", [this]()
	{
		for (auto SemVerString : ValidSemVers)
		{
			It(EscapeTestName(FString::Printf(TEXT("should return the semver string that was used to construct it (%s)"), *SemVerString)), [this, SemVerString]()
			{
				FSemanticVersion SemVer(SemVerString);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_EQUAL(ResultString, SemVerString);
			});
		}
	});

	Describe("TryParseString", [this]()
	{
		for (auto Strictness : TEnumRange<ESemVerParsingStrictness>())
		{
			Describe(FString::Printf(TEXT("with Strictness level %s"), *LexToString(Strictness)), [this, Strictness]()
			{
				for (auto SemVerString : ValidSemVers)
				{
					Describe("should succeed on spec complient semver", [this, Strictness, SemVerString]()
					{
						It(EscapeTestName(FString::Printf(TEXT("%s"), *SemVerString)), [this, Strictness, SemVerString]()
						{
							FSemanticVersion SemVer;
							bool bResult = SemVer.TryParseString(SemVerString, Strictness);
							FString ResultString = SemVer.ToString();
							SPEC_TEST_TRUE(bResult);
							SPEC_TEST_EQUAL(ResultString, SemVerString);
						});
					});
				}
			});
		}

		Describe("with Strictness level Strict", [this]()
		{
			Describe("should fail on spec incompliant semver", [this]()
			{
				for (auto SemVerString : InvalidSemVers)
				{
					It(EscapeTestName(FString::Printf(TEXT("%s"), *SemVerString)), [this, SemVerString]()
					{
						FSemanticVersion SemVer;
						bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Strict);
						FString ResultString = SemVer.ToString();
						SPEC_TEST_FALSE(bResult);
					});
				}
			});
		});

		Describe("with Strictness level Regular", [this]()
		{
			It("should succeed with semvers that have leading zeroes", [this]()
			{
				FString SemVerString = "01.006.010-01.0build.02";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.6.10-1.0build.2");
			});

			It("should succeed with semvers that have special characters in the build metadata", [this]()
			{
				FString SemVerString = "1.0.5+build@meta#data";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.0.5+build@meta#data");
			});

			It("should fail with semvers that have less than three digits", [this]()
			{
				FString SemVerString = "4.0";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_FALSE(bResult);
			});

			It("should fail with semvers that have more than three digits", [this]()
			{
				FString SemVerString = "1.2.3.4";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_FALSE(bResult);
			});

			It("should fail with semvers that contain whitespace", [this]()
			{
				FString SemVerString = "1.2.3.4 ";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Regular);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_FALSE(bResult);
			});
		});

		Describe("with Strictness level Liberal", [this]()
		{
			It("should succeed with semvers that have leading zeroes", [this]()
			{
				FString SemVerString = "01.006.010-01.0build.02";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.6.10-1.0build.2");
			});

			It("should succeed with semvers that have special characters in the build metadata", [this]()
			{
				FString SemVerString = "1.0.5+build@meta#data";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.0.5+build@meta#data");
			});

			It("should succeed with semvers that have a single digits", [this]()
			{
				FString SemVerString = "42";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "42.0.0");
			});

			It("should succeed with semvers that have two digits", [this]()
			{
				FString SemVerString = "4.3";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "4.3.0");
			});

			It("should succeed with semvers that have more than three digits", [this]()
			{
				FString SemVerString = "1.2.3.4";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3-4");
			});

			It("should succeed with semvers that contain whitespace", [this]()
			{
				FString SemVerString = "1.2.3 -build-metadata";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3");
			});

			It("should succeed with semvers that have arbitrary prefixes", [this]()
			{
				FString SemVerString = "Version#=1.2.3-alpha+build";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3-alpha+build");
			});

			It("should succeed with semvers that have arbitrary prefixes or suffixes", [this]()
			{
				FString SemVerString = "The version 1.2.3-alpha+build is the version we need";
				FSemanticVersion SemVer;
				bool bResult = SemVer.TryParseString(SemVerString, ESemVerParsingStrictness::Liberal);
				FString ResultString = SemVer.ToString();
				SPEC_TEST_TRUE(bResult);
				SPEC_TEST_EQUAL(ResultString, "1.2.3-alpha+build");
			});
		});
	});
}

#endif
