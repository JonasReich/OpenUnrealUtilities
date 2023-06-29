// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataAsset.h"

#include "JsonDataAsset/JsonDataAssetGlobals.h"
#include "OUUTestUtilities.h"
#include "TestJsonDataAsset.h"
#include "UObject/Package.h"

#if WITH_AUTOMATION_WORKER

BEGIN_DEFINE_SPEC(FJsonDataAssetSpec, "OpenUnrealUtilities.Runtime.JsonDataAsset.Asset", DEFAULT_OUU_TEST_FLAGS)

	auto MakeTestJsonObject()
	{
		auto JsonObject = MakeShared<FJsonObject>();
		JsonObject->SetStringField("Class", "/Script/OUUTests.TestJsonDataAsset");
		// version should not be required, so we omit it here to keep the test simpler
		return JsonObject;
	}

	auto MakeDataJsonObject_AllValues()
	{
		auto DataObject = MakeShared<FJsonObject>();
		DataObject->SetStringField("string", "Overridden String");
		DataObject->SetStringField(
			"object",
			"/Script/Engine.Material'/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'");

		auto CreateStructJsonObject = [](FString String, FString Object, FString JsonPath) -> TSharedRef<FJsonObject> {
			auto StructObject = MakeShared<FJsonObject>();
			StructObject->SetStringField("string", String);
			StructObject->SetStringField("object", Object);
			StructObject->SetStringField("jsonPath", JsonPath);
			return StructObject;
		};

		auto InlineStructObject = CreateStructJsonObject(
			"Overridden String in Struct",
			"/Script/Engine.Texture2D'/Engine/EngineMaterials/DefaultNormal.DefaultNormal'",
			"/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_NoValuesSet");
		DataObject->SetObjectField("struct", InlineStructObject);

		auto ArrayStruct_Elem0 = CreateStructJsonObject(
			"Struct String 1",
			"/Script/Engine.Material'/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'",
			"/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_NoValuesSet");
		auto ArrayStruct_Elem1 = CreateStructJsonObject(
			"Struct String 2",
			"/Script/Engine.Texture2D'/Engine/EngineMaterials/DefaultNormal.DefaultNormal'",
			"/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_NoValuesSet");
		DataObject->SetArrayField(
			"arrayOfStructs",
			TArray<TSharedPtr<FJsonValue>>{
				MakeShared<FJsonValueObject>(ArrayStruct_Elem0),
				MakeShared<FJsonValueObject>(ArrayStruct_Elem1)});
		return DataObject;
	}

	void TestNoValuesSet(UTestJsonDataAsset * TestAsset)
	{
		if (SPEC_TEST_NOT_NULL(TestAsset))
		{
			SPEC_TEST_EQUAL(TestAsset->String, "Original String (Member)");
			SPEC_TEST_NULL(TestAsset->Object);

			FTestJsonDataAssetStruct ExpectedStruct;
			ExpectedStruct.String = "Original String (Struct)";
			SPEC_TEST_EQUAL(TestAsset->Struct, ExpectedStruct);

			if (SPEC_TEST_EQUAL(TestAsset->ArrayOfStructs.Num(), 1))
			{
				SPEC_TEST_EQUAL(TestAsset->ArrayOfStructs[0].String, "Original String (Array Idx 0)");
			}
		}
	}

	void TestAllValuesSet(UTestJsonDataAsset * TestAsset)
	{
		if (SPEC_TEST_NOT_NULL(TestAsset))
		{
			SPEC_TEST_EQUAL(TestAsset->String, "Overridden String");
			if (SPEC_TEST_NOT_NULL(TestAsset->Object))
			{
				FSoftObjectPath ObjectPath(
					"/Script/Engine.Material'/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'");
				auto* LoadedObject = ObjectPath.TryLoad();
				SPEC_TEST_EQUAL(LoadedObject, TestAsset->Object);
			}

			auto TestStruct = [this](
								  const FTestJsonDataAssetStruct& TestStruct,
								  FString ExpectedString,
								  FString ExpectedObjectPath,
								  FString ExpectedJsonPath) {
				FTestJsonDataAssetStruct ExpectedStruct;
				ExpectedStruct.String = ExpectedString;
				ExpectedStruct.JsonPath = FJsonDataAssetPath::FromPackagePath(ExpectedJsonPath);
				ExpectedStruct.Object = FSoftObjectPath(ExpectedObjectPath).TryLoad();

				SPEC_TEST_FALSE(ExpectedStruct.JsonPath.IsNull());
				SPEC_TEST_NOT_NULL(ExpectedStruct.Object);

				SPEC_TEST_EQUAL(TestStruct, ExpectedStruct);
			};

			TestStruct(
				TestAsset->Struct,
				"Overridden String in Struct",
				"/Script/Engine.Texture2D'/Engine/EngineMaterials/DefaultNormal.DefaultNormal'",
				"/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_NoValuesSet");

			if (SPEC_TEST_EQUAL(TestAsset->ArrayOfStructs.Num(), 2))
			{
				TestStruct(
					TestAsset->ArrayOfStructs[0],
					"Struct String 1",
					"/Script/Engine.Material'/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial'",
					"/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_NoValuesSet");
				TestStruct(
					TestAsset->ArrayOfStructs[1],
					"Struct String 2",
					"/Script/Engine.Texture2D'/Engine/EngineMaterials/DefaultNormal.DefaultNormal'",
					"/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_NoValuesSet");
			}
		}
	}

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

			auto* TestCDO = GetDefault<UTestJsonDataAsset>();
			SPEC_TEST_FALSE(TestCDO->IsFileBasedJsonAsset());
		});

		It("should return true for assets loaded from paths", [this]() {
			auto TestAsset = FJsonDataAssetPath::FromPackagePath(UTestJsonDataAsset::GetTestPath()).LoadSynchronous();
			if (SPEC_TEST_NOT_NULL(TestAsset))
			{
				SPEC_TEST_TRUE(TestAsset->IsFileBasedJsonAsset());
			}
		});
	});

	Describe("GetPath", [this]() {
		It("should return the path it was loaded from", [this]() {
			auto LoadPath = UTestJsonDataAsset::GetTestPath();
			auto TestAsset = FJsonDataAssetPath::FromPackagePath(LoadPath).LoadSynchronous();
			if (SPEC_TEST_NOT_NULL(TestAsset))
			{
				auto ResultPath = TestAsset->GetPath().GetPackagePath();
				SPEC_TEST_EQUAL(ResultPath, LoadPath);
			}
		});

		It("should return a path matching to the generated package path name", [this]() {
			auto LoadPath = UTestJsonDataAsset::GetTestPath();
			auto TestAsset = FJsonDataAssetPath::FromPackagePath(LoadPath).LoadSynchronous();
			if (SPEC_TEST_NOT_NULL(TestAsset))
			{
				auto ResultPath = TestAsset->GetPath().GetPackagePath();
				auto PackagePathName = TestAsset->GetPackage()->GetPathName();
				SPEC_TEST_EQUAL(ResultPath, PackagePathName);
			}
		});

		It("should return a path equal to one constructed from this ptr", [this]() {
			auto LoadPath = UTestJsonDataAsset::GetTestPath();
			auto TestAsset = FJsonDataAssetPath::FromPackagePath(LoadPath).LoadSynchronous();
			if (SPEC_TEST_NOT_NULL(TestAsset))
			{
				auto ResultPath = TestAsset->GetPath();
				auto PathFromThis = FJsonDataAssetPath(TestAsset);
				SPEC_TEST_EQUAL(ResultPath, PathFromThis);
			}
		});
	});

	Describe("Loaded values from disk", [this]() {
		It("should equal to CDO values if the json text does not contain any data values", [this]() {
			auto LoadPath = UTestJsonDataAsset::GetTestPath_NoValuesSet();
			auto TestAsset = Cast<UTestJsonDataAsset>(FJsonDataAssetPath::FromPackagePath(LoadPath).LoadSynchronous());
			TestNoValuesSet(TestAsset);
		});
		It("should equal to expected values all values are set", [this]() {
			auto LoadPath = UTestJsonDataAsset::GetTestPath_AllValuesSet();
			auto TestAsset = Cast<UTestJsonDataAsset>(FJsonDataAssetPath::FromPackagePath(LoadPath).LoadSynchronous());
			TestAllValuesSet(TestAsset);
		});
	});

	Describe("ImportJson", [this]() {
		It("should equal to CDO values if the json object does not have any data members", [this]() {
			auto* TestAsset = NewObject<UTestJsonDataAsset>();
			auto JsonObject = MakeShared<FJsonObject>();
			JsonObject->SetStringField("Class", "/Script/OUUTests.TestJsonDataAsset");
			// version should not be required, so we omit it here to keep the test simpler
			JsonObject->SetObjectField("Data", MakeShared<FJsonObject>());
			TestAsset->ImportJson(JsonObject);
			TestNoValuesSet(TestAsset);
		});
		It("should equal to expected values all values are set", [this]() {
			auto JsonObject = MakeTestJsonObject();
			JsonObject->SetObjectField("Data", MakeDataJsonObject_AllValues());

			auto* TestAsset = NewObject<UTestJsonDataAsset>();
			TestAsset->ImportJson(JsonObject);

			TestAllValuesSet(TestAsset);
		});
	});

	Describe("ExportJson", [this]() {
		It("should return empty data object for object with default values", [this]() {
			auto LoadPath = UTestJsonDataAsset::GetTestPath_NoValuesSet();
			auto TestAsset = Cast<UTestJsonDataAsset>(FJsonDataAssetPath::FromPackagePath(LoadPath).LoadSynchronous());
			if (SPEC_TEST_NOT_NULL(TestAsset))
			{
				auto JsonObject = TestAsset->ExportJson();
				const TSharedPtr<FJsonObject>* JsonDataObject = nullptr;
				if (SPEC_TEST_TRUE(JsonObject->TryGetObjectField("Data", OUT JsonDataObject)))
				{
					auto EmptyJsonObject = MakeShared<FJsonValueObject>(nullptr);
					auto JsonObjectValue = MakeShared<FJsonValueObject>(*EmptyJsonObject);
					SPEC_TEST_TRUE(FJsonValue::CompareEqual(*JsonObjectValue, *EmptyJsonObject));
				}
			}
		});
		It("should return expected data object for object with all set values", [this]() {
			auto LoadPath = UTestJsonDataAsset::GetTestPath_NoValuesSet();
			auto TestAsset = Cast<UTestJsonDataAsset>(FJsonDataAssetPath::FromPackagePath(LoadPath).LoadSynchronous());
			if (SPEC_TEST_NOT_NULL(TestAsset))
			{
				auto JsonObject = TestAsset->ExportJson();
				const TSharedPtr<FJsonObject>* JsonDataObject = nullptr;
				if (SPEC_TEST_TRUE(JsonObject->TryGetObjectField("Data", OUT JsonDataObject)))
				{
					auto EmptyJsonObject = MakeShared<FJsonValueObject>(MakeDataJsonObject_AllValues());
					auto JsonObjectValue = MakeShared<FJsonValueObject>(*EmptyJsonObject);
					SPEC_TEST_TRUE(FJsonValue::CompareEqual(*JsonObjectValue, *EmptyJsonObject));
				}
			}
		});
	});
}

#endif
