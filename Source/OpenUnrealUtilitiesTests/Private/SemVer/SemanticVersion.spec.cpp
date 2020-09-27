// Copyright (c) 2020 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "SemVer/SemanticVersion.h"

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

		It("should initialize the SemVar from a valid SemVar string", [this]()
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
}

#endif
