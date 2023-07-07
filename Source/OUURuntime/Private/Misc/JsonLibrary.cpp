// Copyright (c) 2023 Jonas Reich & Contributors

#include "Misc/JsonLibrary.h"

#include "JsonUtilities.h"
#include "LogOpenUnrealUtilities.h"
#include "Templates/ScopedAssign.h"
#include "Traits/ConditionalType.h"
#include "UObject/TextProperty.h"

namespace OUU::Runtime::Private::JsonLibrary
{
	// The string to return from invalid conversion results.
	const FString InvalidConversionResultString = TEXT("");

} // namespace OUU::Runtime::Private::JsonLibrary

// Use this to bubble information about change status / skip status through the hierarchy.
struct FOUUPropertyJsonResult
{
	bool bSkip = false;
	TSharedPtr<FJsonValue> Value;

	static FOUUPropertyJsonResult Skip() { return FOUUPropertyJsonResult(true, {}); }
	static FOUUPropertyJsonResult Json(TSharedPtr<FJsonValue> Value) { return FOUUPropertyJsonResult(false, Value); }

private:
	FOUUPropertyJsonResult(bool bSkip, TSharedPtr<FJsonValue> Value) : bSkip(bSkip), Value(Value) {}
};

struct FJsonLibraryExportHelper
{
	FJsonLibraryExportHelper(
		int64 InCheckFlags,
		int64 InSkipFlags,
		FOUUJsonLibraryObjectFilter InSubObjectFilter,
		bool bInOnlyModifiedProperties) :
		DefaultCheckFlags(InCheckFlags),
		DefaultSkipFlags(InSkipFlags),
		SubObjectFilter(InSubObjectFilter),
		bOnlyModifiedProperties(bInOnlyModifiedProperties)
	{
	}

	// Export all properties
	int64 DefaultCheckFlags = 0;
	// Don't skip any properties
	int64 DefaultSkipFlags = 0;

	FOUUJsonLibraryObjectFilter SubObjectFilter;

	bool bOnlyModifiedProperties = true;

	mutable int32 RecursionCounter = 0;

	FJsonObjectConverter::CustomExportCallback GetCustomCallback() const
	{
		FJsonObjectConverter::CustomExportCallback CustomCB;
		CustomCB.BindRaw(this, &FJsonLibraryExportHelper::ObjectJsonCallback);
		return CustomCB;
	}

	// Use the same name as FJsonObjectConverter to have compatible exports!
	const FString ObjectClassNameKey = "_ClassName";

	bool SkipPropertyMatchingDefaultValues(FProperty* Property, const void* Value, const void* DefaultValue) const
	{
		if (bOnlyModifiedProperties == false)
		{
			return false;
		}

		if (DefaultValue == nullptr)
		{
			// This property guaranteed to be different.
			// We only pass in nullptr in cases where there is not default to compare (e.g. ptr to array elements in
			// arrays of different size).
			return false;
		}

		if (Property->Identical(Value, DefaultValue))
		{
			return true;
		}

		return false;
	}

	/** Convert property to JSON, assuming either the property is not an array or the value is an individual array
	 * element */
	FOUUPropertyJsonResult ConvertScalarFPropertyToJsonValue(
		FProperty* Property,
		const void* Value,
		const void* DefaultValue,
		int32 Index,
		int64 CheckFlags,
		int64 SkipFlags,
		const FJsonObjectConverter::CustomExportCallback* ExportCb,
		FProperty* OuterProperty) const
	{
		if (SkipPropertyMatchingDefaultValues(Property, Value, DefaultValue))
		{
			return FOUUPropertyJsonResult::Skip();
		}

		// See if there's a custom export callback first, so it can override default behavior
		if (ExportCb && ExportCb->IsBound())
		{
			TSharedPtr<FJsonValue> CustomValue = ExportCb->Execute(Property, Value);
			if (CustomValue.IsValid())
			{
				return FOUUPropertyJsonResult::Json(CustomValue);
			}
			// fall through to default cases
		}

		if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
		{
			// export enums as strings
			UEnum* EnumDef = EnumProperty->GetEnum();
			FString StringValue = EnumDef->GetAuthoredNameStringByValue(
				EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(Value));
			return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueString>(StringValue));
		}
		else if (FNumericProperty* NumericProperty = CastField<FNumericProperty>(Property))
		{
			// see if it's an enum
			UEnum* EnumDef = NumericProperty->GetIntPropertyEnum();
			if (EnumDef != NULL)
			{
				// export enums as strings
				FString StringValue =
					EnumDef->GetAuthoredNameStringByValue(NumericProperty->GetSignedIntPropertyValue(Value));
				return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueString>(StringValue));
			}

			// We want to export numbers as numbers
			if (NumericProperty->IsFloatingPoint())
			{
				return FOUUPropertyJsonResult::Json(
					MakeShared<FJsonValueNumber>(NumericProperty->GetFloatingPointPropertyValue(Value)));
			}
			else if (NumericProperty->IsInteger())
			{
				return FOUUPropertyJsonResult::Json(
					MakeShared<FJsonValueNumber>(NumericProperty->GetSignedIntPropertyValue(Value)));
			}

			// fall through to default
		}
		else if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
		{
			// Export bools as bools
			return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueBoolean>(BoolProperty->GetPropertyValue(Value)));
		}
		else if (FStrProperty* StringProperty = CastField<FStrProperty>(Property))
		{
			return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueString>(StringProperty->GetPropertyValue(Value)));
		}
		else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
		{
			return FOUUPropertyJsonResult::Json(
				MakeShared<FJsonValueString>(TextProperty->GetPropertyValue(Value).ToString()));
		}
		else if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
		{
			TArray<TSharedPtr<FJsonValue>> Out;
			FScriptArrayHelper Helper(ArrayProperty, Value);
			FScriptArrayHelper DefaultHelper(ArrayProperty, DefaultValue);
			for (int32 i = 0, n = Helper.Num(); i < n; ++i)
			{
				void* DefaultElemPtr =
					DefaultValue && DefaultHelper.IsValidIndex(i) ? DefaultHelper.GetRawPtr(i) : nullptr;

				auto Elem = UPropertyToJsonValue(
					ArrayProperty->Inner,
					Helper.GetRawPtr(i),
					DefaultElemPtr,
					CheckFlags & (~CPF_ParmFlags),
					SkipFlags,
					ExportCb,
					ArrayProperty);

				if (Elem.Value.IsValid())
				{
					// add to the array
					Out.Push(Elem.Value);
				}
			}
			return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueArray>(Out));
		}
		else if (FSetProperty* SetProperty = CastField<FSetProperty>(Property))
		{
			TArray<TSharedPtr<FJsonValue>> Out;
			FScriptSetHelper Helper(SetProperty, Value);
			FScriptSetHelper DefaultHelper(SetProperty, DefaultValue);
			for (int32 i = 0, n = Helper.Num(); n; ++i)
			{
				if (Helper.IsValidIndex(i))
				{
					void* DefaultElemPtr =
						DefaultValue && DefaultHelper.IsValidIndex(i) ? DefaultHelper.GetElementPtr(i) : nullptr;

					auto Elem = UPropertyToJsonValue(
						SetProperty->ElementProp,
						Helper.GetElementPtr(i),
						DefaultElemPtr,
						CheckFlags & (~CPF_ParmFlags),
						SkipFlags,
						ExportCb,
						SetProperty);

					if (Elem.Value.IsValid())
					{
						// add to the array
						Out.Push(Elem.Value);
					}

					--n;
				}
			}
			return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueArray>(Out));
		}
		else if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
		{
			TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();

			FScriptMapHelper Helper(MapProperty, Value);
			FScriptMapHelper DefaultHelper(MapProperty, DefaultValue);
			for (int32 i = 0, n = Helper.Num(); n; ++i)
			{
				if (Helper.IsValidIndex(i))
				{
					void* DefaultKeyPtr =
						DefaultValue && DefaultHelper.IsValidIndex(i) ? DefaultHelper.GetKeyPtr(i) : nullptr;
					void* DefaultValuePtr =
						DefaultValue && DefaultHelper.IsValidIndex(i) ? DefaultHelper.GetValuePtr(i) : nullptr;

					auto KeyElement = UPropertyToJsonValue(
						MapProperty->KeyProp,
						Helper.GetKeyPtr(i),
						DefaultKeyPtr,
						CheckFlags & (~CPF_ParmFlags),
						SkipFlags,
						ExportCb,
						MapProperty);
					auto ValueElement = UPropertyToJsonValue(
						MapProperty->ValueProp,
						Helper.GetValuePtr(i),
						DefaultValuePtr,
						CheckFlags & (~CPF_ParmFlags),
						SkipFlags,
						ExportCb,
						MapProperty);

					FString KeyString;
					if (!KeyElement.Value->TryGetString(KeyString))
					{
						MapProperty->KeyProp
							->ExportTextItem_Direct(KeyString, Helper.GetKeyPtr(i), nullptr, nullptr, 0);
						if (KeyString.IsEmpty())
						{
							UE_LOG(
								LogOpenUnrealUtilities,
								Error,
								TEXT("Unable to convert key to string for property %s."),
								*MapProperty->GetAuthoredName())
							KeyString = FString::Printf(TEXT("Unparsed Key %d"), i);
						}
					}

					// Coerce camelCase map keys for Enum/FName properties
					if (CastField<FEnumProperty>(MapProperty->KeyProp)
						|| CastField<FNameProperty>(MapProperty->KeyProp))
					{
						KeyString = FJsonObjectConverter::StandardizeCase(KeyString);
					}
					Out->SetField(KeyString, ValueElement.Value);

					--n;
				}
			}

			return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueObject>(Out));
		}
		else if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
		{
			UScriptStruct::ICppStructOps* TheCppStructOps = StructProperty->Struct->GetCppStructOps();
			// Intentionally exclude the JSON Object wrapper, which specifically needs to export JSON in an object
			// representation instead of a string
			if (StructProperty->Struct != FJsonObjectWrapper::StaticStruct() && TheCppStructOps
				&& TheCppStructOps->HasExportTextItem())
			{
				FString OutValueStr;
				TheCppStructOps->ExportTextItem(OutValueStr, Value, nullptr, nullptr, PPF_None, nullptr);
				return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueString>(OutValueStr));
			}

			TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();
			bool MinimumOneValueSet = false;
			if (UStructToJsonObject(
					StructProperty->Struct,
					Value,
					DefaultValue,
					Out,
					OUT MinimumOneValueSet,
					CheckFlags & (~CPF_ParmFlags),
					SkipFlags,
					ExportCb))
			{
				return (MinimumOneValueSet || bOnlyModifiedProperties == false)
					? FOUUPropertyJsonResult::Json(MakeShared<FJsonValueObject>(Out))
					: FOUUPropertyJsonResult::Skip();
			}
		}
		else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
		{
			// Instanced properties should be copied by value, while normal UObject* properties should output as asset
			// references
			UObject* Object = ObjectProperty->GetObjectPropertyValue(Value);
			if (Object
				&& (ObjectProperty->HasAnyPropertyFlags(CPF_PersistentInstance)
					|| (OuterProperty && OuterProperty->HasAnyPropertyFlags(CPF_PersistentInstance))))
			{
				TSharedRef<FJsonObject> Out = MakeShared<FJsonObject>();

				Out->SetStringField(ObjectClassNameKey, Object->GetClass()->GetPathName());
				bool MinimumOneValueSet = false;
				if (UStructToJsonObject(
						ObjectProperty->GetObjectPropertyValue(Value)->GetClass(),
						Object,
						Object->GetClass()->GetDefaultObject(),
						Out,
						OUT MinimumOneValueSet,
						CheckFlags,
						SkipFlags,
						ExportCb))
				{
					UObject* DefaultObject = DefaultValue ? ObjectProperty->GetObjectPropertyValue(DefaultValue) : nullptr;
					// No class or different class
					const bool bDifferentClass =
						DefaultObject == nullptr || Object->GetClass() != DefaultObject->GetClass();

					TSharedRef<FJsonValueObject> JsonObject = MakeShared<FJsonValueObject>(Out);
					JsonObject->Type = EJson::Object;
					return (MinimumOneValueSet || bOnlyModifiedProperties == false || bDifferentClass)
						? FOUUPropertyJsonResult::Json(JsonObject)
						: FOUUPropertyJsonResult::Skip();
				}
			}
			else
			{
				FString StringValue;
				Property->ExportTextItem_Direct(StringValue, Value, nullptr, nullptr, PPF_None);
				return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueString>(StringValue));
			}
		}
		else
		{
			// Default to export as string for everything else
			FString StringValue;
			Property->ExportTextItem_Direct(StringValue, Value, NULL, NULL, PPF_None);
			return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueString>(StringValue));
		}

		// invalid
		return FOUUPropertyJsonResult::Json(TSharedPtr<FJsonValue>());
	}

	FOUUPropertyJsonResult UPropertyToJsonValue(
		FProperty* Property,
		const void* Value,
		const void* DefaultValue,
		int64 CheckFlags,
		int64 SkipFlags,
		const FJsonObjectConverter::CustomExportCallback* ExportCb,
		FProperty* OuterProperty = nullptr) const
	{
		if (SkipPropertyMatchingDefaultValues(Property, Value, DefaultValue))
		{
			return FOUUPropertyJsonResult::Skip();
		}

		if (Property->ArrayDim == 1)
		{
			return ConvertScalarFPropertyToJsonValue(
				Property,
				Value,
				DefaultValue,
				0,
				CheckFlags,
				SkipFlags,
				ExportCb,
				OuterProperty);
		}

		TArray<TSharedPtr<FJsonValue>> Array;
		for (int Index = 0; Index != Property->ArrayDim; ++Index)
		{
			auto ArrayElement = ConvertScalarFPropertyToJsonValue(
				Property,
				(char*)Value + Index * Property->ElementSize,
				DefaultValue ? (char*)DefaultValue + Index + Property->ElementSize : nullptr,
				Index,
				CheckFlags,
				SkipFlags,
				ExportCb,
				OuterProperty);

			// We can't really skip individual array elements, can we?
			// Also, we already assume, something is changed in here, so we have to serialize the entire array
			ensure(ArrayElement.bSkip == false);

			Array.Add(ArrayElement.Value);
		}
		return FOUUPropertyJsonResult::Json(MakeShared<FJsonValueArray>(Array));
	}

	bool UStructToJsonAttributes(
		const UStruct* StructDefinition,
		const void* Struct,
		const void* DefaultStruct,
		TMap<FString, TSharedPtr<FJsonValue>>& OutJsonAttributes,
		bool& OutMinimumOneValueSet,
		int64 CheckFlags,
		int64 SkipFlags,
		const FJsonObjectConverter::CustomExportCallback* ExportCb) const
	{
		OutMinimumOneValueSet = false;

		if (SkipFlags == 0)
		{
			// If we have no specified skip flags, skip deprecated, transient and skip serialization by default when
			// writing
			SkipFlags |= CPF_Deprecated | CPF_Transient;
		}

		if (StructDefinition == FJsonObjectWrapper::StaticStruct())
		{
			// Just copy it into the object
			const FJsonObjectWrapper* ProxyObject = (const FJsonObjectWrapper*)Struct;

			if (ProxyObject->JsonObject.IsValid())
			{
				OutJsonAttributes = ProxyObject->JsonObject->Values;
				OutMinimumOneValueSet = true;
			}
			return true;
		}

		for (TFieldIterator<FProperty> It(StructDefinition); It; ++It)
		{
			FProperty* Property = *It;

			// Check to see if we should ignore this property
			if (CheckFlags != 0 && !Property->HasAnyPropertyFlags(CheckFlags))
			{
				continue;
			}
			if (Property->HasAnyPropertyFlags(SkipFlags))
			{
				continue;
			}

			FString VariableName = FJsonObjectConverter::StandardizeCase(Property->GetAuthoredName());
			const void* Value = Property->ContainerPtrToValuePtr<uint8>(Struct);
			const void* DefaultValue = DefaultStruct ? Property->ContainerPtrToValuePtr<uint8>(DefaultStruct) : nullptr;

			// convert the property to a FJsonValue
			auto PropertyResult = UPropertyToJsonValue(Property, Value, DefaultValue, CheckFlags, SkipFlags, ExportCb);
			if (PropertyResult.bSkip)
				continue;

			OutMinimumOneValueSet = true;

			TSharedPtr<FJsonValue> JsonValue = PropertyResult.Value;
			if (!JsonValue.IsValid())
			{
				FFieldClass* PropClass = Property->GetClass();
				UE_LOG(
					LogOpenUnrealUtilities,
					Error,
					TEXT("UStructToJsonAttributes - Unhandled property type '%s': %s"),
					*PropClass->GetName(),
					*Property->GetPathName());
				return false;
			}

			// set the value on the output object
			OutJsonAttributes.Add(VariableName, JsonValue);
		}

		return true;
	}

	// Implementation copied from FJsonObjectConverter::ObjectJsonCallback
	// Modified to support stop class
	TSharedPtr<FJsonValue> ObjectJsonCallback(FProperty* Property, const void* Value) const { return {}; }

	bool UStructToJsonObject(
		const UStruct* StructDefinition,
		const void* Struct,
		const void* DefaultStruct,
		TSharedRef<FJsonObject> OutJsonObject,
		bool& OutMinimumOneValueSet,
		int64 CheckFlags,
		int64 SkipFlags,
		const FJsonObjectConverter::CustomExportCallback* ExportCb) const
	{
		return UStructToJsonAttributes(
			StructDefinition,
			Struct,
			DefaultStruct,
			OutJsonObject->Values,
			OutMinimumOneValueSet,
			CheckFlags,
			SkipFlags,
			ExportCb);
	}

	TSharedPtr<FJsonObject> ConvertStructToJsonObject(const void* Data, const void* DefaultData, const UStruct* Struct)
	{
		FJsonObjectConverter::CustomExportCallback CustomCB = GetCustomCallback();
		TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		bool MinimumOneValueSet = false;
		if (UStructToJsonObject(
				Struct,
				Data,
				DefaultData,
				OUT JsonObject,
				OUT MinimumOneValueSet,
				DefaultCheckFlags,
				DefaultSkipFlags,
				&CustomCB))
		{
			return JsonObject;
		}
		return TSharedPtr<FJsonObject>();
	}

	TSharedPtr<FJsonObject> ConvertObjectToJsonObject(const UObject* Object)
	{
		FJsonObjectConverter::CustomExportCallback CustomCB = GetCustomCallback();
		TSharedRef<FJsonObject> JsonObject = MakeShared<FJsonObject>();
		bool MinimumOneValueSet = false;
		if (UStructToJsonObject(
				Object->GetClass(),
				Object,
				Object->GetClass()->GetDefaultObject(),
				OUT JsonObject,
				OUT MinimumOneValueSet,
				DefaultCheckFlags,
				DefaultSkipFlags,
				&CustomCB))
		{
			return JsonObject;
		}
		return TSharedPtr<FJsonObject>();
	}

	TSharedPtr<FJsonValue> ConvertPropertyToJsonValue(const void* Data, const void* DefaultData, FProperty* Property)
	{
		FJsonObjectConverter::CustomExportCallback CustomCB = GetCustomCallback();
		auto Result = UPropertyToJsonValue(Property, Data, DefaultData, DefaultCheckFlags, DefaultSkipFlags, &CustomCB);
		if (Result.Value)
		{
			return Result.Value;
		}
		return TSharedPtr<FJsonValueNull>();
	}

	template <
		bool bPrettyPrint,
		class PrintPolicy =
			TConditionalType<bPrettyPrint, TPrettyJsonPrintPolicy<TCHAR>, TCondensedJsonPrintPolicy<TCHAR>>::Type>
	bool UStructToJsonObjectStringInternal(const TSharedRef<FJsonObject>& JsonObject, FString& OutJsonString)
	{
		constexpr int32 Indent = 4;
		TSharedRef<TJsonWriter<TCHAR, PrintPolicy>> JsonWriter =
			TJsonWriterFactory<TCHAR, PrintPolicy>::Create(&OutJsonString, Indent);
		bool bSuccess = FJsonSerializer::Serialize(JsonObject, JsonWriter);
		JsonWriter->Close();
		return bSuccess;
	}

	template <bool bPrettyPrint>
	FString ConvertObjectToString(const UObject* Object)
	{
		TSharedPtr<FJsonObject> JsonObject = ConvertObjectToJsonObject(Object);
		if (JsonObject.IsValid())
		{
			FString JsonString;
			if (UStructToJsonObjectStringInternal<bPrettyPrint>(JsonObject.ToSharedRef(), OUT JsonString))
			{
				return JsonString;
			}
			else
			{
				UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("ConvertObjectToString - Unable to write out JSON"));
			}
		}

		return OUU::Runtime::Private::JsonLibrary::InvalidConversionResultString;
	}
};

TSharedPtr<FJsonObject> UOUUJsonLibrary::UStructToJsonObject(
	const void* Data,
	const void* DefaultData,
	UStruct* Struct,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */,
	bool bOnlyModifiedProperties /* = false */)
{
	if (!Data || !IsValid(Struct))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid struct TO Json object"));
		return nullptr;
	}

	FJsonLibraryExportHelper Helper{CheckFlags, SkipFlags, SubObjectFilter, bOnlyModifiedProperties};
	return Helper.ConvertStructToJsonObject(Data, DefaultData, Struct);
}

TSharedPtr<FJsonObject> UOUUJsonLibrary::UObjectToJsonObject(
	const UObject* Object,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */,
	bool bOnlyModifiedProperties /* = false */)
{
	if (!IsValid(Object))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid object TO Json object"));
		return nullptr;
	}

	FJsonLibraryExportHelper Helper{CheckFlags, SkipFlags, SubObjectFilter, bOnlyModifiedProperties};
	return Helper.ConvertObjectToJsonObject(Object);
}

TSharedPtr<FJsonValue> UOUUJsonLibrary::UPropertyToJsonValue(
	const void* PropertyData,
	const void* DefaultPropertyData,
	FProperty* Property,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */,
	bool bOnlyModifiedProperties /* = false */)
{
	if (!PropertyData || !Property)
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid property TO Json value"));
		return nullptr;
	}

	FJsonLibraryExportHelper Helper{CheckFlags, SkipFlags, SubObjectFilter, bOnlyModifiedProperties};
	return Helper.ConvertPropertyToJsonValue(PropertyData, DefaultPropertyData, Property);
}

bool UOUUJsonLibrary::JsonValueToUProperty(
	TSharedRef<FJsonValue> JsonValue,
	void* PropertyData,
	FProperty* Property,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */)
{
	return FJsonObjectConverter::JsonValueToUProperty(JsonValue, Property, PropertyData, CheckFlags, SkipFlags);
}

FString UOUUJsonLibrary::UObjectToJsonString(
	const UObject* Object,
	FOUUJsonLibraryObjectFilter SubObjectFilter,
	int64 CheckFlags /* = 0 */,
	int64 SkipFlags /* = 0 */,
	bool bOnlyModifiedProperties /* = false */)
{
	if (!IsValid(Object))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Failed to convert invalid object TO Json string"));
		return OUU::Runtime::Private::JsonLibrary::InvalidConversionResultString;
	}

	FJsonLibraryExportHelper Helper{CheckFlags, SkipFlags, SubObjectFilter, bOnlyModifiedProperties};

	constexpr bool bPrettyPrint = true;
	return Helper.ConvertObjectToString<bPrettyPrint>(Object);
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
