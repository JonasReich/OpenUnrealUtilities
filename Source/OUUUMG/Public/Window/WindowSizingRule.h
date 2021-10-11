// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "WindowSizingRule.generated.h"

/**
* Enum to describe how windows are sized
* This is the same as ESizingRule, but blueprint exposed
*/
UENUM(BlueprintType)
enum class EWindowSizingRule : uint8
{
	/* The windows size fixed and cannot be resized **/
	FixedSize,

	/** The window size is computed from its content and cannot be resized by users */
	Autosized,

	/** The window can be resized by users */
	UserSized,
};
