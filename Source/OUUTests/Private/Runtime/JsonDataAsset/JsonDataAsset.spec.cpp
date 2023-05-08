// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAsset.h"

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

BEGIN_DEFINE_SPEC(FJsonDataAssetSpec, "OpenUnrealUtilities.Runtime.JsonDataAsset.Asset", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FJsonDataAssetSpec)

void FJsonDataAssetSpec::Define()
{
	Describe("UtilFuncs", [this]() {
		Describe("PackageToObjectName", [this]() {
			It("should return the package name for a package path", [this]() {
				auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(TEXT("/JsonData/Folder/PackageName"));
				SPEC_TEST_EQUAL(ObjectName, TEXT("PackageName"));
			});

			It("should return an empty string for a string that does not contain slashes", [this]() {
				auto ObjectName = OUU::Runtime::JsonData::PackageToObjectName(TEXT("ObjectName"));
				SPEC_TEST_EQUAL(ObjectName, TEXT(""));
			});
		});

		Describe("Package/Source Conversion", [this]() {
			Describe("in Read mode", [this]() {
				It("should return the same path", [this]() {
					auto PackagePath = TEXT("/JsonData/Folder/PackageName");
					auto SourcePath =
						OUU::Runtime::JsonData::PackageToSourceFull(PackagePath, EJsonDataAccessMode::Read);
					auto PackagePathResult =
						OUU::Runtime::JsonData::SourceFullToPackage(SourcePath, EJsonDataAccessMode::Read);
					SPEC_TEST_EQUAL(PackagePath, PackagePathResult);
				});
			});
			Describe("in Write mode", [this]() {
				It("should return the same path", [this]() {
					auto PackagePath = TEXT("/JsonData/Folder/PackageName");
					auto SourcePath =
						OUU::Runtime::JsonData::PackageToSourceFull(PackagePath, EJsonDataAccessMode::Write);
					auto PackagePathResult =
						OUU::Runtime::JsonData::SourceFullToPackage(SourcePath, EJsonDataAccessMode::Write);
					SPEC_TEST_EQUAL(PackagePath, PackagePathResult);
				});
			});
		});
	});

	Describe("IsFileBasedJsonAsset", [this]() {
		It("should return false for CDOs", [this]() {
			auto* CDO = GetDefault<UJsonDataAsset>();
			SPEC_TEST_FALSE(CDO->IsFileBasedJsonAsset());
		});
	});

	// #TODO-OUU Add plugin json folders so we can create plugin / test content, so we can test save/load functionality.
	// I don't want to introduce test assets into project content.
}

#endif
