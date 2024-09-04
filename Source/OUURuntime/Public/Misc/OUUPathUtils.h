// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Misc/AsciiSet.h"

namespace OUU::Runtime::PathUtils
{
	/**
	 * @returns copy of FileName without any characters not a-zA-Z0-9 range or '_' or '-'.
	 */
	OUURUNTIME_API FString SanitizeAsciiFileName(FStringView FileName);
} // namespace OUU::Runtime::PathUtils
