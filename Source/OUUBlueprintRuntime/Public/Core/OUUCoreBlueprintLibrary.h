// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUCoreBlueprintLibrary.generated.h"

/**
 * Core engine/UObject functionality that is for some reason not blueprint exposed via Kismet functions, e.g
 * GetDefaultObject(), etc. Does not expand upon existing C++ functionality but merely makes it available for blueprint
 * use.
 */
UCLASS()
class UOUUCoreBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** @returns the mutable class default object of the specified class. Proceed with caution! */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Class", Meta = (DeterminesOutputType = Class))
	static UObject* GetClassDefaultObject(TSubclassOf<UObject> Class);
	
	/** @returns the mutable class default object of the objects class. Proceed with caution! */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Class")
	static UObject* GetClassDefaultObjectFromObject(const UObject* Object);

	/**
	 * Attempts to get the world from a world context object.
	 * @returns nullptr if no world was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|World")
	static UWorld* TryGetWorldFromObject(const UObject* WorldContextObject);

	/**
	 * Attempts to get the world from the current blueprint context.
	 * Functionally the same as TryGetWorldFromObject(), but this version auto-populates the WorldContextObject
	 * parameter in Blueprints.
	 * @returns nullptr if not world was found.
	 */
	UFUNCTION(
		BlueprintPure,
		Category = "Open Unreal Utilities|World",
		meta = (DisplayName = "Try Get World", WorldContext = "WorldContextObject"))
	static UWorld* TryGetWorldFromObject_K2(const UObject* WorldContextObject);

	/**
	 * Mark an object as modified.
	 * If we are currently recording into the transaction buffer (undo/redo),
	 * save a copy of this object into the buffer and mark the package as needing to be saved.
	 */
	UFUNCTION(BlueprintCallable,Category="OUU|Utilities|Core")
	static void ModifyObject(UObject* Object);

	/** Converts a TopLevelAssetPath to a string */
	UFUNCTION(
		BlueprintPure,
		meta = (DisplayName = "To String (Top Level Asset Path)", CompactNodeTitle = "->", BlueprintAutocast),
		Category = "Utilities|Top Level Asset Path")
	static FString Conv_TopLevelAssetPathToString(const FTopLevelAssetPath& InPath);

	/** Converts an FString to a TopLevelAssetPath */
	UFUNCTION(
		BlueprintPure,
		meta = (DisplayName = "To Top Level Asset Path (String)", CompactNodeTitle = "->", BlueprintAutocast),
		Category = "Utilities|Top Level Asset Path")
	static FTopLevelAssetPath Conv_StringToTopLevelAssetPath(const FString& InPath);

	/** Converts a UClass to a TopLevelAssetPath */
	UFUNCTION(
		BlueprintPure,
		meta = (DisplayName = "To Top Level Asset Path (Class)", CompactNodeTitle = "->", BlueprintAutocast),
		Category = "Utilities|Top Level Asset Path")
	static FTopLevelAssetPath Conv_ClassToTopLevelAssetPath(const UClass* InClass);
};
