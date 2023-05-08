// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUBlueprintPropertyFlags.generated.h"

/**
 * These flags are copies of EPropertyFlags exposed for Blueprint use.
 * Do not use as bitmasks directly - these enum cases must be converted using the blueprint functions from
 * UOUUBlueprintPropertyFlagsLibrary.
 *
 * Please check that source enum for more detailed documentation on general flags usage and individual flags purpose.
 */
UENUM(BlueprintType)
enum class EOUUBlueprintPropertyFlags : uint8
{
	None,

	// DO NOT USE DIRECTLY!!!
	Group_IndividualFlags UMETA(DisplayName = "-- Individual Flags --"),

	// Property is user-settable in the editor.
	Edit,
	// This is a constant function parameter
	ConstParm,
	// This property can be read by blueprint code
	BlueprintVisible,
	// Object can be exported with actor.
	ExportObject,
	// This property cannot be modified by blueprint code
	BlueprintReadOnly,
	// Property is relevant to network replication.
	Net,
	// Indicates that elements of an array can be modified, but its size cannot be changed.
	EditFixedSize,
	// Function/When call parameter.
	Parm,
	// Value is copied out after function call.
	OutParm,
	// memset is fine for construction
	ZeroConstructor,
	// Return value.
	ReturnParm,
	// Disable editing of this property on an archetype/sub-blueprint
	DisableEditOnTemplate,
	// Property is transient: shouldn't be saved or loaded, except for Blueprint CDOs.
	Transient,
	// Property should be loaded/saved as permanent profile.
	Config,
	// Disable editing on an instance of this class
	DisableEditOnInstance,
	// Property is uneditable in the editor.
	EditConst,
	// Load config from base class, not subclass.
	GlobalConfig,
	// Property is a component references.
	InstancedReference,

	// Property should always be reset to the default value during any
	// type of duplication (copy/paste, binary duplication, etc.)
	DuplicateTransient,

	// Property should be serialized for save games, this is only checked for
	// game-specific archives with ArIsSaveGame
	SaveGame,
	// Hide clear (and browse) button.
	NoClear,
	// Value is passed by reference; OutParam and Param should also be set.
	ReferenceParm,
	// MC Delegates only.  Property should be exposed for assigning in blueprint code
	BlueprintAssignable,
	// Property is deprecated.  Read it from an archive, but don't save it.
	Deprecated,
	// If this is set, then the property can be memcopied instead of
	// CopyCompleteValue / CopySingleValue
	IsPlainOldData,
	// Not replicated. For non replicated properties in replicated structs
	RepSkip,
	// Notify actors when a property is replicated
	RepNotify,
	// interpolatable property for use with matinee
	Interp,
	// Property isn't transacted
	NonTransactional,
	// Property should only be loaded in the editor
	EditorOnly,
	// No destructor
	NoDestructor,
	// Only used for weak pointers, means the export type is autoweak
	AutoWeak,
	// Property contains component references.
	ContainsInstancedReference,
	// asset instances will add properties with this flag to the asset registry automatically
	AssetRegistrySearchable,
	// The property is visible by default in the editor details view
	SimpleDisplay,
	// The property is advanced and not visible by default in the editor details view
	AdvancedDisplay,
	// property is protected from the perspective of script
	Protected,
	// MC Delegates only.  Property should be exposed for calling in blueprint code
	BlueprintCallable,
	// MC Delegates only.  This delegate accepts (only in blueprint)
	// only events with BlueprintAuthorityOnly.
	BlueprintAuthorityOnly,
	// Property shouldn't be exported to text format (e.g. copy/paste)
	TextExportTransient,
	// Property should only be copied in PIE
	NonPIEDuplicateTransient,
	// Property is exposed on spawn
	ExposeOnSpawn,
	// A object referenced by the property is duplicated like a
	// component. (Each actor should have an own instance.)
	PersistentInstance,
	// Property was parsed as a wrapper class like TSubclassOf<T>,
	// FScriptInterface etc., rather than a USomething*
	UObjectWrapper,
	// This property can generate a meaningful hash value.
	HasGetValueTypeHash,
	// Public native access specifier
	NativeAccessSpecifierPublic,
	// Protected native access specifier
	NativeAccessSpecifierProtected,
	// Private native access specifier
	NativeAccessSpecifierPrivate,
	// Property shouldn't be serialized, can still be exported to text
	SkipSerialization,

	// DO NOT USE DIRECTLY!!!
	Group_MaskPresets UMETA(DisplayName = "-- Mask Presets --"),

	/** All Native Access Specifier flags */
	NativeAccessSpecifiers,
	/** All parameter flags */
	ParmFlags,
	/** Flags that are propagated to properties inside containers */
	PropagateToArrayInner,
	PropagateToMapValue,
	PropagateToMapKey,
	PropagateToSetElement,
	/** The flags that should never be set on interface properties */
	InterfaceClearMask,
	/** All the properties that can be stripped for final release console builds */
	DevelopmentAssets,
	/** All the properties that should never be loaded or saved */
	ComputedFlags,
	/** Mask of all property flags */
	AllFlags,

	/// HIDDEN META FLAGS FOR STATIC VALIDATION
	META_NumFlagsAndGroups UMETA(Hidden),
	// Adjust manually to match number of group headings above
	META_NumGroups = 2 UMETA(Hidden),
	META_NumFlags = META_NumFlagsAndGroups - META_NumGroups UMETA(Hidden)
};

UCLASS()
class UOUUBlueprintPropertyFlagsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Property Flags")
	static int64 CreatePropertyFlagsMask(TSet<EOUUBlueprintPropertyFlags> Flags);

	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Property Flags")
	static TSet<EOUUBlueprintPropertyFlags> BreakPropertyFlagsMask(int64 Flags);
};
