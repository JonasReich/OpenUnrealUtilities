// Copyright (c) 2022 Jonas Reich

#include "Misc/JsonLibrary.h"

#include "JsonUtilities.h"
#include "LogOpenUnrealUtilities.h"

namespace JsonLibrary_Private
{
	// The string to return from invalid conversion results.
	const FString InvalidConversionResultString = TEXT("");
} // namespace JsonLibrary_Private

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

	FString ConvertObjectToString(UObject* Object)
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
		return bSuccess ? Result : JsonLibrary_Private::InvalidConversionResultString;
	}
};

FString UOUUJsonLibrary::UObjectToJsonString(
	UObject* Object,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags,
	int64 SkipFlags)
{
	if (!IsValid(Object))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid object TO Json string"));
		return JsonLibrary_Private::InvalidConversionResultString;
	}

	FJsonLibraryExportHelper Helper{CheckFlags, SkipFlags, SubObjectFilter};
	return Helper.ConvertObjectToString(Object);
}

bool UOUUJsonLibrary::JsonStringToUObject(UObject* Object, FString String, int64 CheckFlags, int64 SkipFlags)
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
