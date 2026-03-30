// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

namespace OUU::Developer::ActorMapWindow::Style
{
	/**
	 * Hard coded editor colors, do not update with editor style config,
	 * but I did not want to deal with that at this time...
	 */
	inline FSlateColorBrush DarkGrey(FColor(6, 6, 6, 255));
	inline FSlateColorBrush MediumGrey(FColor(13, 13, 13, 255));
	inline FSlateColorBrush White(FColor::White);
	inline FColor LabelBackgroundColor(0, 0, 0, 200);
} // namespace OUU::Developer::ActorMapWindow::Style
