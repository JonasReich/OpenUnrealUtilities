// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "GameplayTags/LiteralGameplayTag.h"
#include "GameplayTags/TypedGameplayTag.h"

#include "SampleGameplayTags.generated.h"

/**
 * Literal gameplay tag hierarchy used for OUU Tests that involve gameplay tags.
 * These tags are added as native tags from code, so you don't have to declare them via the DefaultGameplayTags.ini file
 * and you can still use them in any project.
 * Please be aware that these tags should only be used inside this module, because they are not present when this module
 * is not loaded/included.
 */
OUU_DECLARE_GAMEPLAY_TAGS_START(
	OUURUNTIME_API,
	FSampleGameplayTags,
	"OUUTestTags",
	"Test tags for OpenUnrealUtilties tests. Do not use for anything else!",
	ELiteralGameplayTagFlags::Default)
	OUU_GTAG(Foo, "Foo is a leaf tag that has no children");
	OUU_GTAG_GROUP_START(Bar, "Bar has children")
		OUU_GTAG_GROUP_START(Alpha, "Alpha is the greek letter equivalent of A")
			OUU_GTAG(One, "Number One. Inside Alpha")
			OUU_GTAG(Two, "Number Two. Inside Alpha")
		OUU_GTAG_GROUP_END(Alpha)
		OUU_GTAG(Beta, "Beta is the greek letter equivalent of B")
		OUU_GTAG(Gamma, "Gamma is the greek letter equivalent of G, but comes at the place of C")
		OUU_GTAG(Delta, "Delta is the greek letter equivalent of D")
	OUU_GTAG_GROUP_END(Bar)
	OUU_GTAG(Baz, "A third tag on this level")
OUU_DECLARE_GAMEPLAY_TAGS_END(FSampleGameplayTags)

OUU_DECLARE_GAMEPLAY_TAGS_START(
	OUURUNTIME_API,
	FSampleGameplayTags_Nested,
	"OUUTestTags.NestedTreeRoot",
	"Test tags for OpenUnrealUtilties tests. Do not use for anything else!",
	ELiteralGameplayTagFlags::Default)
OUU_DECLARE_GAMEPLAY_TAGS_END(FSampleGameplayTags_Nested)

/**
 * This literal tag does NOT auto-registered as native tags.
 * Used to check that the bAutoAddNativeTag parameter actually makes a difference.
 */
OUU_DECLARE_GAMEPLAY_TAGS_START(
	OUURUNTIME_API,
	FSampleGameplayTags_NotRegsitered,
	"UUTestTag_NotRegistered",
	"This tag should never be registered",
	ELiteralGameplayTagFlags::Default ^ ELiteralGameplayTagFlags::AutoRegister)
OUU_DECLARE_GAMEPLAY_TAGS_END(FSampleGameplayTags_NotRegsitered)

/**
 * Test tags for OpenUnrealUtilties tests. Do not use for anything else!
 * Tests assume this has OUUTestTags.Bar and OUUTestTags.Baz as native tags and no additional root tags.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUSampleBarTag : public FTypedGameplayTag_Base
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_GAMEPLAY_TAG(FOUUSampleBarTag, FSampleGameplayTags::Bar, FSampleGameplayTags::Baz)
};
