// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUBlueprintObjectFlags.generated.h"

/**
 * These flags are copies of EObjectFlags exposed for Blueprint use.
 * Do not use as bitmasks directly - these enum cases must be converted using the blueprint functions from
 * UOUUBlueprintObjectFlagsLibrary.
 *
 * Please check that source enum for more detailed documentation on general flags usage and individual flags purpose.
 */
UENUM(BlueprintType)
enum class EOUUBlueprintObjectFlags : uint8
{
	// No flags, used to avoid a cast
	NoFlags = 0,

	// DO NOT USE DIRECTLY!!!
	GROUP_ObjectType UMETA(DisplayName = "-- Object Types --"),

	// Object is visible outside its package.
	Public,
	// Keep object around for editing even if unreferenced.
	Standalone,
	// Object (UField) will be marked as native on construction (DO NOT USE THIS FLAG in
	MarkAsNative,
	// Object is transactional.
	Transactional,
	// This object is its class's default object
	ClassDefaultObject,
	// This object is a template for another object - treat like a class default object
	ArchetypeObject,
	// Don't save object.
	Transient,

	// DO NOT USE DIRECTLY!!!
	GROUP_GC UMETA(DisplayName = "-- Garbage Collection --"),

	// Object will be marked as root set on construction and not be garbage collected, even if unreferenced
	MarkAsRootSet,
	// This is a temp user flag for various utilities that need to use the garbage
	// collector. The garbage collector itself does not interpret it.
	TagGarbageTemp,

	// DO NOT USE DIRECTLY!!!
	GROUP_Lifetime UMETA(DisplayName = "-- Lifetime --"),

	// This object has not completed its initialization process. Cleared when
	// ~FObjectInitializer completes
	NeedInitialization,
	// During load, indicates object needs loading.
	NeedLoad,
	// Keep this object during garbage collection because it's still being used by the cooker
	KeepForCooker,
	// Object needs to be postloaded.
	NeedPostLoad,
	// During load, indicates that the object still needs to instance subobjects
	// and fixup serialized component references
	NeedPostLoadSubobjects,
	// Object has been consigned to oblivion due to its owner package being reloaded,
	// and a newer version currently exists
	NewerVersionExists,
	// BeginDestroy has been called on the object.
	BeginDestroyed,
	// FinishDestroy has been called on the object.
	FinishDestroyed,

	// DO NOT USE DIRECTLY!!!
	GROUP_Misc UMETA(DisplayName = "-- Misc --"),

	// Flagged on UObjects that are used to create UClasses (e.g. Blueprints) while
	// they are regenerating their UClass on load (See FLinkerLoad::CreateExport()), as
	// well as UClass objects in the midst of being created
	BeingRegenerated,
	// Flagged on subobjects that are defaults
	DefaultSubObject,
	// Flagged on UObjects that were loaded
	WasLoaded,
	// Do not export object to text form (e.g. copy/paste). Generally used for
	// sub-objects that can be regenerated from data in their parent object.
	TextExportTransient,
	// Object has been completely serialized by linkerload at least once. DO NOT USE THIS
	// FLAG, It should be replaced with RF_WasLoaded.
	LoadCompleted,
	// Archetype of the object can be in its super class
	InheritableComponentTemplate,
	// Object should not be included in any type of duplication (copy/paste, binary duplication, etc.)
	DuplicateTransient,
	// References to this object from persistent function frame are handled as strong ones.
	StrongRefOnFrame,
	// Object should not be included for duplication unless it's being duplicated for a PIE session
	NonPIEDuplicateTransient,
	// This object was constructed during load and will be loaded shortly
	WillBeLoaded,
	// This object has an external package assigned and should look it up when
	// getting the outermost package
	HasExternalPackage,

	// Allocated from a ref-counted page shared with other UObjects
	AllocatedInSharedPage,

	// DO NOT USE DIRECTLY!!!
	GROUP_MaskPresets UMETA(DisplayName = "-- Mask Presets --"),

	// All flags, used mainly for error checking
	AllFlags,
	// Flags to load from unreal asset files
	Load,
	// Sub-objects will inherit these flags from their SuperObject
	PropagateToSubObjects,

	/// HIDDEN META FLAGS FOR STATIC VALIDATION
	META_NumFlagsAndGroups UMETA(Hidden),
	// Adjust manually to match number of group headings above
	META_NumGroups = 5 UMETA(Hidden),
	META_NumFlags = META_NumFlagsAndGroups - META_NumGroups UMETA(Hidden)
};

UCLASS()
class UOUUBlueprintObjectFlagsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Object Flags")
	static int64 CreateObjectFlagsMask(const TSet<EOUUBlueprintObjectFlags>& Flags);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Object Flags")
	static TSet<EOUUBlueprintObjectFlags> BreakObjectFlagsMask(int64 Flags);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Object Flags")
	static TSet<EOUUBlueprintObjectFlags> GetObjectFlagsSet(const UObject* Object);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Object Flags")
	static int64 GetObjectFlagsMask(const UObject* Object);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Object Flags")
	static bool ObjectHasAnyFlags(const UObject* Object, const TSet<EOUUBlueprintObjectFlags>& Flags);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Object Flags")
	static bool ObjectHasAllFlags(const UObject* Object, const TSet<EOUUBlueprintObjectFlags>& Flags);
};
