// Copyright (c) 2022 Jonas Reich

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
