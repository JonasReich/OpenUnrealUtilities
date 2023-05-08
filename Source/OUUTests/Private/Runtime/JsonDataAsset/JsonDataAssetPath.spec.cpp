// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonDataAssetPath.h"

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

BEGIN_DEFINE_SPEC(FJsonDataAssetPathSpec, "OpenUnrealUtilities.Runtime.JsonDataAsset.Path", DEFAULT_OUU_TEST_FLAGS)
	FString ImportText;
	FJsonDataAssetPath Path;

	void ImportAndTestPathIsSetToTestAsset()
	{
		const TCHAR* ImportTextPtr = *ImportText;
		Path.ImportTextItem(ImportTextPtr, 0, nullptr, nullptr);
		FString ExpectedPath = TEXT("/JsonData/Folder/Asset");
		SPEC_TEST_EQUAL(Path.GetPackagePath(), ExpectedPath);

		FString ExportResult;
		Path.ExportTextItem(OUT ExportResult, FJsonDataAssetPath(), nullptr, 0, nullptr);
		SPEC_TEST_EQUAL(ExportResult, ExpectedPath);
	}

END_DEFINE_SPEC(FJsonDataAssetPathSpec)

void FJsonDataAssetPathSpec::Define()
{
	BeforeEach([this]() { Path = FJsonDataAssetPath(); });

	Describe("GetSetPath", [this]() {
		It("should allow setting from package path", [this]() {
			Path.SetPackagePath(TEXT("/JsonData/Folder/Asset"));
			SPEC_TEST_EQUAL(Path.GetPackagePath(), TEXT("/JsonData/Folder/Asset"));
		});
	});

	Describe("ImportTextItem", [this]() {
		It("should allow setting from package path", [this]() {
			ImportText = TEXT("/JsonData/Folder/Asset");
			ImportAndTestPathIsSetToTestAsset();
		});

		It("should allow setting from object path", [this]() {
			ImportText = TEXT("/JsonData/Folder/Asset.Asset");
			ImportAndTestPathIsSetToTestAsset();
		});

		It("should allow setting from soft object path", [this]() {
			ImportText = TEXT("Script.JsonDataType'/JsonData/Folder/Asset.Asset'");
			ImportAndTestPathIsSetToTestAsset();
		});
	});

	Describe("IsNull", [this]() {
		It("should return true for default initialized paths", [this]() { SPEC_TEST_TRUE(Path.IsNull()); });
		It("should return false for any syntactically valid path - even if the asset doesn't exist", [this]() {
			Path.SetPackagePath(TEXT("/JsonData/Folder/Asset"));
			SPEC_TEST_FALSE(Path.IsNull());
		});
	});
}

#endif
