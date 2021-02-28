// Copyright (c) 2021 Jonas Reich

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "OUUCoreBlueprintLibrary.generated.h"

/**
 * Core engine/UObject functionality that is for some reason not blueprint exposed via Kismet functions, e.g GetDefaultObject(), etc.
 * Does not expand upon existing C++ functionality but merely makes it available for blueprint use. 
 */
UCLASS()
class UOUUCoreBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** @returns the mutable class default object of the specified class. Proceed with caution! */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Class")
	static UObject* GetClassDefaultObject(TSubclassOf<UObject> Class);

	/** @returns the mutable class default object of the objects class. Proceed with caution! */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Class")
    static UObject* GetClassDefaultObjectFromObject(UObject* Object);

	/**
	 * Attempts to get the world from a world context object.
	 * Returns nullptr if no world was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|World")
	static UWorld* TryGetWorldFromObject(UObject* WorldContextObject);

	/**
	 * Attempts to get the world from the current blueprint context.
	 * Functionally the same as TryGetWorldFromObject(), but this version auto-populates the WorldContextObject parameter in Blueprints.
	 * Returns nullptr if not world was found.
	 */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|World", meta = (DisplayName = "Try Get World", WorldContext = "WorldContextObject"))
	static UWorld* TryGetWorldFromObject_K2(UObject* WorldContextObject);
};
