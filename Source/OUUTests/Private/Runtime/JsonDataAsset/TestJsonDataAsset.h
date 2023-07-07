// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "JsonDataAsset/JsonDataAsset.h"

#include "TestJsonDataAsset.generated.h"

USTRUCT()
struct FTestJsonDataAssetStruct
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	FString String = "Original String (Struct)";

	UPROPERTY(EditAnywhere)
	UObject* Object = nullptr;

	UPROPERTY(EditAnywhere)
	FJsonDataAssetPath JsonPath;
};

FORCEINLINE bool operator==(const FTestJsonDataAssetStruct& A, const FTestJsonDataAssetStruct& B)
{
	return A.String == B.String && A.Object == B.Object && A.JsonPath == B.JsonPath;
}

FORCEINLINE bool operator!=(const FTestJsonDataAssetStruct& A, const FTestJsonDataAssetStruct& B)
{
	return !(A == B);
}

UCLASS(DefaultToInstanced, EditInlineNew)
class UTestJsonDataAsset_InstancedObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere)
	int32 Integer;
};

UCLASS(Hidden)
class UTestJsonDataAsset : public UJsonDataAsset
{
	GENERATED_BODY()

public:
	UTestJsonDataAsset()
	{
		ArrayOfStructs.AddDefaulted();
		ArrayOfStructs[0].String = "Original String (Array Idx 0)";
	}

	UPROPERTY(EditAnywhere)
	FString String = "Original String (Member)";

	UPROPERTY(EditAnywhere)
	UObject* Object = nullptr;

	UPROPERTY(EditAnywhere)
	FTestJsonDataAssetStruct Struct;

	UPROPERTY(EditAnywhere)
	TArray<FTestJsonDataAssetStruct> ArrayOfStructs;

	UPROPERTY(EditAnywhere, Instanced)
	UTestJsonDataAsset_InstancedObject* InstancedObject;

	UPROPERTY(EditAnywhere, Instanced)
	TArray<UTestJsonDataAsset_InstancedObject*> ArrayOfInstancedObjects;

	// Get path to any test asset (no requirements to contained data)
	static FString GetTestPath() { return GetTestPath_NoValuesSet(); }

	// Get path to an asset that has no values changed compared to CDO.
	static FString GetTestPath_NoValuesSet()
	{
		return TEXT("/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_NoValuesSet");
	}

	// Get path to an asset that has all values changed compared to CDO.
	static FString GetTestPath_AllValuesSet()
	{
		return TEXT("/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_AllValuesSet");
	}

	/*
	* #TODO-OUU Implement tests for delta serialization
	// Get path to an asset that has some values changed compared to CDO.
	// This asset is in the gitignore and p4ignore and can must be created/altered before test execution.
	static FString GetTestPath_SomeValuesSet()
	{
		return TEXT("/JsonData/Plugins/OpenUnrealUtilities/Tests/TestAsset_SomeValuesSet");
	}
	*/
};
