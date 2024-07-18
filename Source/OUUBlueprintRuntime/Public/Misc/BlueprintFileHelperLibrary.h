// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "BlueprintFileHelperLibrary.generated.h"

/** Expose FFileHelper functionality to Blueprint */
UCLASS()
class OUUBLUEPRINTRUNTIME_API UOUUBlueprintFileHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	static bool LoadFileToString(const FString& FilePath, FString& Result)
	{
		return FFileHelper::LoadFileToString(OUT Result, *FilePath);
	}

	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	static bool SaveStringToFile(const FString& String, const FString& FilePath)
	{
		return FFileHelper::SaveStringToFile(String, *FilePath, FFileHelper::EEncodingOptions::ForceUTF8);
	}
};
