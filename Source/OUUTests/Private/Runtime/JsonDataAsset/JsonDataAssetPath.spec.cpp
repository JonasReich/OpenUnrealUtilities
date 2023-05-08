// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataAssetPath.h"

#include "OUUTestUtilities.h"
#include "TestJsonDataAsset.h"

#if WITH_AUTOMATION_WORKER

	// This json path doesn't point to an actual test asset
	#define FAKE_JSON_PATH TEXT("/JsonData/Folder/Asset")

BEGIN_DEFINE_SPEC(FJsonDataAssetPathSpec, "OpenUnrealUtilities.Runtime.JsonDataAsset.Path", DEFAULT_OUU_TEST_FLAGS)
	FString ImportText;
	FJsonDataAssetPath Path;

	void ImportAndTestPathIsSetToTestAsset()
	{
		const TCHAR* ImportTextPtr = *ImportText;
		Path.ImportTextItem(ImportTextPtr, 0, nullptr, nullptr);
		SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);

		FString ExportResult;
		Path.ExportTextItem(OUT ExportResult, FJsonDataAssetPath(), nullptr, 0, nullptr);
		SPEC_TEST_EQUAL(ExportResult, FAKE_JSON_PATH);
	}

END_DEFINE_SPEC(FJsonDataAssetPathSpec)

void FJsonDataAssetPathSpec::Define()
{
	BeforeEach([this]() {
		ImportText = "";
		Path = FJsonDataAssetPath();
	});

	Describe("GetSetPath", [this]() {
		It("should allow constructing from package path", [this]() {
			Path = FJsonDataAssetPath::FromPackagePath(FAKE_JSON_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});

		It("should allow setting from package path", [this]() {
			Path.SetPackagePath(FAKE_JSON_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});
	});

	Describe("ImportTextItem", [this]() {
		It("should allow setting from package path", [this]() {
			ImportText = FAKE_JSON_PATH;
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
		It("should return true for default initialized paths", [this]() {
			Path = FJsonDataAssetPath();
			SPEC_TEST_TRUE(Path.IsNull());
		});
		It("should return false for any syntactically valid path - even if the asset doesn't exist", [this]() {
			Path.SetPackagePath(FAKE_JSON_PATH);
			SPEC_TEST_FALSE(Path.IsNull());
		});
	});

	Describe("LoadSynchronous", [this]() {
		It("should return nullptr on null path", [this]() {
			Path = FJsonDataAssetPath();
			UJsonDataAsset* LoadedObject = Path.LoadSynchronous();
			SPEC_TEST_NULL(LoadedObject);
		});
		It("should return nullptr on invalid path", [this]() {
			// invalid as in "asset does not exist"
			// syntactically invalid paths may still result in errors.
			Path = FJsonDataAssetPath::FromPackagePath(FAKE_JSON_PATH);
			UJsonDataAsset* LoadedObject = Path.LoadSynchronous();
			SPEC_TEST_NULL(LoadedObject);
		});
		It("should return valid ptr on valid path", [this]() {
			Path = FJsonDataAssetPath::FromPackagePath(UTestJsonDataAsset::GetTestPath());
			UJsonDataAsset* LoadedObject = Path.LoadSynchronous();
			SPEC_TEST_NOT_NULL(LoadedObject);
		});
	});

	Describe("ResolveObject", [this]() {
		It("should return nullptr on null path", [this]() {
			Path = FJsonDataAssetPath();
			UJsonDataAsset* LoadedObject = Path.ResolveObject();
			SPEC_TEST_NULL(LoadedObject);
		});
		It("should return nullptr on invalid path", [this]() {
			// invalid as in "asset does not exist"
			// syntactically invalid paths may still result in errors.
			Path = FJsonDataAssetPath::FromPackagePath(FAKE_JSON_PATH);
			UJsonDataAsset* LoadedObject = Path.ResolveObject();
			SPEC_TEST_NULL(LoadedObject);
		});
		It("should return valid ptr on valid path if object is already in memory", [this]() {
			// Load with a different path
			auto OtherPathToLoad = FJsonDataAssetPath::FromPackagePath(UTestJsonDataAsset::GetTestPath());
			auto* LoadedObjectFromOriginalPath = OtherPathToLoad.LoadSynchronous();

			Path = FJsonDataAssetPath::FromPackagePath(UTestJsonDataAsset::GetTestPath());
			UJsonDataAsset* LoadedObject = Path.ResolveObject();
			SPEC_TEST_NOT_NULL(LoadedObject);
			SPEC_TEST_EQUAL(LoadedObject, LoadedObjectFromOriginalPath);
		});
	});
}

	#undef FAKE_JSON_PATH

#endif
