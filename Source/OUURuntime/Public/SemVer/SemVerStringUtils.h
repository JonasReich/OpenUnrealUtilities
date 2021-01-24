// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SemVerStringUtils.generated.h"

UCLASS()
class OUURUNTIME_API USemVerStringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure)
	static bool IsValidSemanticVersion(const FString& InString, ESemVerParsingStrictness ParsingStrictness);
};

using FSemVerStringUtils = USemVerStringLibrary;
