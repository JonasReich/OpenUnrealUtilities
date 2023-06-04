// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataAssetPath.h"

#include "OUUTestUtilities.h"
#include "TestJsonDataAsset.h"

#if WITH_AUTOMATION_WORKER

	// This json path doesn't point to an actual test asset
	#define FAKE_JSON_PATH		   TEXT("/JsonData/Folder/Asset")
	#define FAKE_SHORT_OBJECT_PATH TEXT("/JsonData/Folder/Asset.Asset")
	#define FAKE_SOFT_OBJECT_PATH  TEXT("Script.JsonDataType'/JsonData/Folder/Asset.Asset'")

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

	Describe("FromPackagePath", [this]() {
		It("should allow constructing from package path", [this]() {
			Path = FJsonDataAssetPath::FromPackagePath(FAKE_JSON_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});
	});

	Describe("SetPackagePath", [this]() {
		It("should allow setting from package path", [this]() {
			Path.SetPackagePath(FAKE_JSON_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});
	});

	Describe("SetObjectPath", [this]() {
		It("should allow setting from short object path", [this]() {
			Path.SetObjectPath(FAKE_SHORT_OBJECT_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});

		It("should allow setting from soft object path", [this]() {
			Path.SetObjectPath(FAKE_SOFT_OBJECT_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});
	});

	Describe("SetFromString", [this]() {
		It("should allow setting from package path", [this]() {
			Path.SetFromString(FAKE_JSON_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});

		It("should allow setting from short object path", [this]() {
			Path.SetFromString(FAKE_SHORT_OBJECT_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});

		It("should allow setting from soft object path", [this]() {
			Path.SetFromString(FAKE_SOFT_OBJECT_PATH);
			SPEC_TEST_EQUAL(Path.GetPackagePath(), FAKE_JSON_PATH);
		});
	});

	Describe("ImportTextItem", [this]() {
		It("should allow setting from package path", [this]() {
			ImportText = FAKE_JSON_PATH;
			ImportAndTestPathIsSetToTestAsset();
		});

		It("should allow setting from short object path", [this]() {
			ImportText = FAKE_SHORT_OBJECT_PATH;
			ImportAndTestPathIsSetToTestAsset();
		});

		It("should allow setting from soft object path", [this]() {
			ImportText = FAKE_SOFT_OBJECT_PATH;
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

		It("should return true for a path that was Reset", [this]() {
			Path.SetPackagePath(FAKE_JSON_PATH);
			Path.Reset();
			SPEC_TEST_TRUE(Path.IsNull());
		});

		It("should return true for a path that was assigned an empty string path", [this]() {
			Path.SetPackagePath(FAKE_JSON_PATH);
			Path.SetPackagePath("");
			SPEC_TEST_TRUE(Path.IsNull());
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

			{
				// All of these are warnings that only appear on the first unsuccessful load.
				// AddExpectedError("File .*Asset.json does not exist");
				// AddExpectedError("Failed to load", EAutomationExpectedErrorFlags::Contains, 2);
				// AddExpectedError("Failed to find object");
			}

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
	#undef FAKE_SHORT_OBJECT_PATH
	#undef FAKE_SOFT_OBJECT_PATH

#endif
