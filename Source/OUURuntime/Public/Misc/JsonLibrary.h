// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Dom/JsonObject.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "JsonLibrary.generated.h"

USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUJsonLibraryObjectFilter
{
	GENERATED_BODY()
public:
	// Exclude object properties to objects of these classes
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UClass*> ExcludeClasses;

	// At which depth of nested sub-objects the export should stop
	// Limit of 0 means no nested UObjects are serialized,
	// a limit of 1 means only the first nested objects are serialized, etc.
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 SubObjectDepthLimit = 3;
};

UCLASS()
class OUURUNTIME_API UOUUJsonLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	static TSharedPtr<FJsonObject> UStructToJsonObject(
		const void* Data,
		const void* DefaultData,
		UStruct* Struct,
		FOUUJsonLibraryObjectFilter SubObjectFilter,
		int64 CheckFlags = 0,
		int64 SkipFlags = 0,
		bool bOnlyModifiedProperties = false);
	template <typename StructT>
	static TSharedPtr<FJsonObject> UStructToJsonObject(
		const StructT* StructData,
		FOUUJsonLibraryObjectFilter SubObjectFilter,
		int64 CheckFlags = 0,
		int64 SkipFlags = 0,
		bool bOnlyModifiedProperties = false);

	static TSharedPtr<FJsonObject> UObjectToJsonObject(
		const UObject* Object,
		FOUUJsonLibraryObjectFilter SubObjectFilter,
		int64 CheckFlags = 0,
		int64 SkipFlags = 0,
		bool bOnlyModifiedProperties = false);

	static TSharedPtr<FJsonValue> UPropertyToJsonValue(
		const void* PropertyData,
		const void* DefaultPropertyData,
		FProperty* Property,
		FOUUJsonLibraryObjectFilter SubObjectFilter,
		int64 CheckFlags = 0,
		int64 SkipFlags = 0,
		bool bOnlyModifiedProperties = false);

	static bool JsonValueToUProperty(
		TSharedRef<FJsonValue> JsonValue,
		void* PropertyData,
		FProperty* Property,
		int64 CheckFlags = 0,
		int64 SkipFlags = 0);

	/**
	 * Create a json string from an objects properties.
	 * Parses all properties down to the limit/filter settings provided in SubObjectFilter.
	 */
	UFUNCTION(BlueprintCallable)
	static FString UObjectToJsonString(
		const UObject* Object,
		FOUUJsonLibraryObjectFilter SubObjectFilter,
		int64 CheckFlags = 0,
		int64 SkipFlags = 0,
		bool bOnlyModifiedProperties = false);

	UFUNCTION(BlueprintCallable)
	static bool JsonStringToUObject(UObject* Object, FString String, int64 CheckFlags = 0, int64 SkipFlags = 0);
};

template <typename StructT>
TSharedPtr<FJsonObject> UOUUJsonLibrary::UStructToJsonObject(
	const StructT* StructData,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */,
	bool bOnlyModifiedProperties /* = false */)
{
	const StructT Default;
	return UStructToJsonObject(
		StructData,
		&Default,
		StructT::StaticStruct(),
		SubObjectFilter,
		CheckFlags,
		SkipFlags,
		bOnlyModifiedProperties);
}