// Copyright (c) 2022 Jonas Reich

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

namespace OUU::Runtime
{
	using SemVerStringUtils = USemVerStringLibrary;
} // namespace OUU::Runtime

using FSemVerStringUtils UE_DEPRECATED(
	5.0,
	"FSemVerStringUtils has been deprecated in favor of OUU::Runtime::SemVerStringUtils.") =
	OUU::Runtime::SemVerStringUtils;
