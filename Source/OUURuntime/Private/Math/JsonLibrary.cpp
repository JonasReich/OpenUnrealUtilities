// Copyright (c) 2022 Jonas Reich

#include "Misc/JsonLibrary.h"

#include "JsonUtilities.h"
#include "LogOpenUnrealUtilities.h"

namespace OUU::Runtime::Private::JsonLibrary
{
	// The string to return from invalid conversion results.
	const FString InvalidConversionResultString = TEXT("");
} // namespace OUU::Runtime::Private::JsonLibrary

struct FJsonLibraryExportHelper
{
	FJsonLibraryExportHelper(int64 InCheckFlags, int64 InSkipFlags, FOUUJsonLibraryObjectFilter InSubObjectFilter) :
		CheckFlags(InCheckFlags), SkipFlags(InSkipFlags), SubObjectFilter(InSubObjectFilter)
	{
	}

	// Export all properties
	int64 CheckFlags = 0;
	// Don't skip any properties
	int64 SkipFlags = 0;

	FOUUJsonLibraryObjectFilter SubObjectFilter;

	// How many tabs to add to the json serializer
	int32 Indent = 0;
	bool bPrettyPrint = true;

	mutable int32 RecursionCounter = 0;

	FJsonObjectConverter::CustomExportCallback GetCustomCallback() const
	{
		FJsonObjectConverter::CustomExportCallback CustomCB;
		CustomCB.BindRaw(this, &FJsonLibraryExportHelper::ObjectJsonCallback);
		return CustomCB;
	}

	// Implementation copied from FJsonObjectConverter::ObjectJsonCallback
	// Modified to support stop class
	TSharedPtr<FJsonValue> ObjectJsonCallback(FProperty* Property, const void* Value) const
	{
		TScopeCounter ScopeCounter{RecursionCounter};

		// Apply maximum recursion limit
		if (RecursionCounter >= SubObjectFilter.SubObjectDepthLimit)
			return {};

		FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property);
		// We only need to handle object properties in this callback.
		if (!ObjectProperty)
			return {};

		// We are taking Transient to mean we don't want to serialize to Json either
		// (could make a new flag if necessary)
		if (ObjectProperty->HasAnyFlags(RF_Transient))
			return {};

		auto* Object = ObjectProperty->GetObjectPropertyValue(Value);
		if (IsValid(Object))
		{
			for (auto ExcludeClass : SubObjectFilter.ExcludeClasses)
			{
				if (Object->GetClass()->IsChildOf(ExcludeClass))
					return {};
			}
		}

		TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();

		FJsonObjectConverter::CustomExportCallback CustomCB = GetCustomCallback();

		void** PtrToValuePtr = (void**)Value;

		if (FJsonObjectConverter::UStructToJsonObject(
				ObjectProperty->PropertyClass,
				(*PtrToValuePtr),
				Out,
				0,
				0,
				&CustomCB))
		{
			return MakeShared<FJsonValueObject>(Out);
		}
		return {};
	};

	TSharedPtr<FJsonObject> ConvertObjectToJsonObject(const UObject* Object)
	{
		FJsonObjectConverter::CustomExportCallback CustomCB = GetCustomCallback();
		TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		if (FJsonObjectConverter::UStructToJsonObject(
				Object->GetClass(),
				Object,
				OUT JsonObject,
				CheckFlags,
				SkipFlags,
				&CustomCB))
		{
			return JsonObject;
		}
		return TSharedPtr<FJsonObject>();
	}

	FString ConvertObjectToString(const UObject* Object)
	{
		auto* Class = Object->GetClass();
		FString Result;
		FJsonObjectConverter::CustomExportCallback CustomCB = GetCustomCallback();
		bool bSuccess = FJsonObjectConverter::UStructToJsonObjectString(
			Class,
			Object,
			OUT Result,
			CheckFlags,
			SkipFlags,
			Indent,
			&CustomCB,
			bPrettyPrint);
		return bSuccess ? Result : OUU::Runtime::Private::JsonLibrary::InvalidConversionResultString;
	}
};

TSharedPtr<FJsonObject> UOUUJsonLibrary::UObjectToJsonObject(
	const UObject* Object,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */)
{
	if (!IsValid(Object))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid object TO Json object"));
		return nullptr;
	}

	FJsonLibraryExportHelper Helper{CheckFlags, SkipFlags, SubObjectFilter};
	return Helper.ConvertObjectToJsonObject(Object);
}

FString UOUUJsonLibrary::UObjectToJsonString(
	const UObject* Object,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */)
{
	if (!IsValid(Object))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid object TO Json string"));
		return OUU::Runtime::Private::JsonLibrary::InvalidConversionResultString;
	}

	FJsonLibraryExportHelper Helper{CheckFlags, SkipFlags, SubObjectFilter};
	return Helper.ConvertObjectToString(Object);
}

bool UOUUJsonLibrary::JsonStringToUObject(
	UObject* Object,
	FString String,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */)
{
	if (!IsValid(Object))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid object FROM Json string"));
		return false;
	}

	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(String);
	if (!FJsonSerializer::Deserialize(JsonReader, JsonObject) || !JsonObject.IsValid())
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("JsonStringToUObject - Unable to parse json=[%s]"), *String);
		return false;
	}
	if (!FJsonObjectConverter::JsonObjectToUStruct(
			JsonObject.ToSharedRef(),
			Object->GetClass(),
			Object,
			CheckFlags,
			SkipFlags))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("JsonStringToUObject - Unable to deserialize. json=[%s]"),
			*String);
		return false;
	}
	return true;
}
