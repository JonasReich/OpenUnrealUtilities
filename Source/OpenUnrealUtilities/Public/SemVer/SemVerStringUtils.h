// Copyright (c) 2020 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SemVerStringUtils.generated.h"

UCLASS()
class OPENUNREALUTILITIES_API USemVerStringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure)
	static bool IsValidSemanticVersion(const FString& InString);
};

using FSemVerStringUtils = USemVerStringLibrary;
