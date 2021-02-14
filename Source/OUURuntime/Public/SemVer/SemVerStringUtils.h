// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "SemVerStringUtils.generated.h"

enum class ESemVerParsingStrictness : uint8;

UCLASS()
class OUURUNTIME_API USemVerStringLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Semantic Versioning")
	static bool IsValidSemanticVersion(const FString& InString, ESemVerParsingStrictness ParsingStrictness);
};

using FSemVerStringUtils = USemVerStringLibrary;
