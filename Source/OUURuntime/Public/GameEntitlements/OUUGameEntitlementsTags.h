// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "GameplayTags/TypedGameplayTag.h"

#include "OUUGameEntitlementsTags.generated.h"

OUU_DECLARE_GAMEPLAY_TAGS_START(
	OUURUNTIME_API,
	FOUUGameEntitlementTags,
	"GameEntitlements",
	"Tags for the game entitlements system from the OpenUnrealUtilities plugin")
	OUU_GTAG(
		Module,
		"Individual items a user/session may be entitled to access that can be locked/unlocked",
		ParentTagType::Flags | EFlags::AllowContentChildTags)
	OUU_GTAG(
		Collection,
		"Meta combination of modules that can be controlled at once",
		ParentTagType::Flags | EFlags::AllowContentChildTags)
	OUU_GTAG(
		Version,
		"A 'version' of the game that has a predefined selection of modules that are available",
		ParentTagType::Flags | EFlags::AllowContentChildTags)
OUU_DECLARE_GAMEPLAY_TAGS_END(FOUUGameEntitlementTags)

/** Individual items a user/session may be entitled to access that can be locked/unlocked. */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUGameEntitlementModule : public FTypedGameplayTag_Base
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_GAMEPLAY_TAG(
		FOUUGameEntitlementModule,
		FOUUGameEntitlementTags::Module,
		FOUUGameEntitlementTags::Collection)
};

/** Meta combination of modules that can be controlled at once */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUGameEntitlementCollection : public FTypedGameplayTag_Base
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_GAMEPLAY_TAG(FOUUGameEntitlementCollection, FOUUGameEntitlementTags::Collection)
};

/** A 'version' of the game that has a predefined selection of modules that are available */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUGameEntitlementVersion : public FTypedGameplayTag_Base
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_GAMEPLAY_TAG(FOUUGameEntitlementVersion, FOUUGameEntitlementTags::Version)
};
