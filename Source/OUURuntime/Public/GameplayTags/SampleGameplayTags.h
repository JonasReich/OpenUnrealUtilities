// Copyright (c) 2022 Jonas Reich

#pragma once

#include "GameplayTags/LiteralGameplayTag.h"

/**
 * Literal gameplay tag hierarchy used for OUU Tests that involve gameplay tags.
 * These tags are added as native tags from code, so you don't have to declare them via the DefaultGameplayTags.ini file
 * and you can still use them in any project.
 * Please be aware that these tags should only be used inside this module, because they are not present when this module
 * is not loaded/included.
 */
DECLARE_OUU_GAMEPLAY_TAGS(FSampleGameplayTags, true, OUURUNTIME_API)
	OUU_GTAG_GROUP_START(OUUTestTags, "Test tags for OpenUnrealUtilties tests. Do not use for anything else!")
		OUU_GTAG(Foo, "Foo is a leaf tag that has no children");
		OUU_GTAG_GROUP_START(Bar, "Bar has children")
			OUU_GTAG(Alpha, "Alpha is the greek letter equivalent of A")
			OUU_GTAG(Beta, "Beta is the greek letter equivalent of B")
			OUU_GTAG(Gamma, "Gamma is the greek letter equivalent of G, but comes at the place of C")
			OUU_GTAG(Delta, "Delta is the greek letter equivalent of D")
		OUU_GTAG_GROUP_END(Bar)
	OUU_GTAG_GROUP_END(OUUTestTags)
	DECLARE_OUU_GAMEPLAY_TAGS_END

		/**
		 * This literal tag does NOT auto-registered as native tags.
		 * Used to check that the bAutoAddNativeTag parameter actually makes a difference.
		 */
		DECLARE_OUU_GAMEPLAY_TAGS(FSampleGameplayTags_NotRegsitered, false, OUURUNTIME_API)
			OUU_GTAG(OUUTestTag_NotRegistered, "");
			DECLARE_OUU_GAMEPLAY_TAGS_END
