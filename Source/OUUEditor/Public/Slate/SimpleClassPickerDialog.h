// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "UObject/ObjectMacros.h"

namespace OUU::Editor
{
	/**
	 * Opens a class picker dialog that allows selecting any subclass of ParentClass, except for those with
	 * DisallowedClassFlags.
	 * @returns nullptr if the user cancels out of the dialog
	 */
	UClass* OpenSimpleClassPickerDialog(UClass* ParentClass, EClassFlags DisallowedClassFlags, const FText TitleText);
} // namespace OUU::Editor
