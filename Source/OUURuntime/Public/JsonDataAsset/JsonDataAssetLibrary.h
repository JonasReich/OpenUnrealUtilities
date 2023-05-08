// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "JsonDataAsset/JsonDataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"

#include "JsonDataAssetLibrary.generated.h"

/**
 * Blueprint utility functions for json data assets.
 * These should not be required in C++ code.
 */
UCLASS()
class OUURUNTIME_API UJsonDataAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	//////////////////////////////////////////////////////////////////////////
	// FJsonDataAssetPath Members
	//////////////////////////////////////////////////////////////////////////

	/** Try to resolve the path in memory, do NOT load if not found. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Json Data Asset")
	static UJsonDataAsset* ResolveObjectJsonDataAsset(const FJsonDataAssetPath& Path);

	/** Try to resolve the path in memory, LOAD asset if not found. */
	UFUNCTION(
		BlueprintPure,
		Category = "Open Unreal Utilities|Json Data Asset",
		meta = (DisplayName = "Load Synchronous"))
	static UJsonDataAsset* LoadJsonDataAssetSynchronous(const FJsonDataAssetPath& Path);

	/** Try to resolve the path in memory, LOAD asset if not found. ALWAYS reload members from json source. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Json Data Asset")
	static UJsonDataAsset* ForceReloadJsonDataAsset(const FJsonDataAssetPath& Path);

	// Get the object pointed to by the given FSoftJsonDataAssetPtr if it is currently loaded.
	UFUNCTION(BlueprintPure, Meta = (DeterminesOutputType = Class), Category = "Open Unreal Utilities|Json Data Asset")
	static UJsonDataAsset* GetSoftJsonDataAssetPtr(const FSoftJsonDataAssetPtr& Ptr, TSubclassOf<UJsonDataAsset> Class);

	// Get the object pointed to by the given FSoftJsonDataAssetPtr (and load it if it isn't loaded yet)
	UFUNCTION(
		BlueprintPure,
		Meta = (DeterminesOutputType = Class, DisplayName = "Load Synchronous"),
		Category = "Open Unreal Utilities|Json Data Asset")
	static UJsonDataAsset* LoadJsonDataAssetPtrSyncronous(
		const FSoftJsonDataAssetPtr& Ptr,
		TSubclassOf<UJsonDataAsset> Class);

	// Get the object pointed to by the given FJsonDataAssetPtr.
	UFUNCTION(BlueprintPure, Meta = (DeterminesOutputType = Class), Category = "Open Unreal Utilities|Json Data Asset")
	static UJsonDataAsset* GetJsonDataAssetPtr(const FJsonDataAssetPtr& Ptr, TSubclassOf<UJsonDataAsset> Class);

	//////////////////////////////////////////////////////////////////////////
	// Blueprint Conversion
	//////////////////////////////////////////////////////////////////////////

	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToRawPtr (SoftJsonDataAssetPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "RawPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static UJsonDataAsset* Conv_SoftJsonDataAssetPtrToRawPtr(const FSoftJsonDataAssetPtr& InPtr);
	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToSoftJsonDataAssetPtr (RawPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "SoftJsonDataAssetPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static FSoftJsonDataAssetPtr Conv_RawPtrToSoftJsonDataAssetPtr(UJsonDataAsset* InPtr);

	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToRawPtr (JsonDataAssetPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "RawPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static UJsonDataAsset* Conv_JsonDataAssetPtrToRawPtr(const FJsonDataAssetPtr& InPtr);
	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToJsonDataAssetPtr (RawPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "JsonDataAssetPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static FJsonDataAssetPtr Conv_RawPtrToJsonDataAssetPtr(UJsonDataAsset* InPtr);
};
