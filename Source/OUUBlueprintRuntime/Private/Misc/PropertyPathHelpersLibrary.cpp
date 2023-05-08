// Copyright (c) 2023 Jonas Reich & Contributors

#include "Misc/PropertyPathHelpersLibrary.h"

#include "PropertyPathHelpers.h"

FString UOUUPropertyPathHelpersLibrary::GetPropertyValueAsString(UObject* Object, const FString& PropertyPath)
{
	FString ExportedValue;
	PropertyPathHelpers::GetPropertyValueAsString(Object, PropertyPath, OUT ExportedValue);
	return ExportedValue;
}

bool UOUUPropertyPathHelpersLibrary::SetPropertyValueFromString(
	UObject* Object,
	const FString& PropertyPath,
	const FString& ValueAsString)
{
	return PropertyPathHelpers::SetPropertyValueFromString(Object, PropertyPath, ValueAsString);
}
