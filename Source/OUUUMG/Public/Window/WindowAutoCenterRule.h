// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "WindowAutoCenterRule.generated.h"

/**
 * Enum to describe how to auto-center an SWindow.
 * (This is the same as EAutoCenter, but blueprint/property exposed).
 */
UENUM(BlueprintType)
enum class EWindowAutoCenterRule : uint8
{
	/** Don't auto-center the window */
	None,

	/** Auto-center the window on the primary work area */
	PrimaryWorkArea,

	/** Auto-center the window on the preferred work area, determined using GetPreferredWorkArea() */
	PreferredWorkArea,
};
