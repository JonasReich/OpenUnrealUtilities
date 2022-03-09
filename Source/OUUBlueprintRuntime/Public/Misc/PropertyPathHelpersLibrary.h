// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "PropertyPathHelpersLibrary.generated.h"

UCLASS()
class UOUUPropertyPathHelpersLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Property Path Helpers")
	static FString GetPropertyValueAsString(UObject* Object, const FString& PropertyPath);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Property Path Helpers")
	static bool SetPropertyValueFromString(UObject* Object, const FString& PropertyPath, const FString& ValueAsString);
};
