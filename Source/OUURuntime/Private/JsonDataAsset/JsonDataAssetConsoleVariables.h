// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "HAL/IConsoleManager.h"

// Console variable declarations for json data asset runtime.
// Not in Public/ folder, because they should only be used directly by the system internal code.
// See cpp file for more detailed descriptions on each variable.
namespace OUU::Runtime::JsonData::Private
{
	// Bool switches
	extern TAutoConsoleVariable<bool> CVar_SeparateSourceMountRoot;
	extern TAutoConsoleVariable<bool> CVar_ImportAllAssetsOnStartup;
	extern TAutoConsoleVariable<bool> CVar_PurgeAssetCacheOnStartup;
	extern TAutoConsoleVariable<bool> CVar_IgnoreLoadErrorsDuringStartupImport;
	extern TAutoConsoleVariable<bool> CVar_UseFastNetSerialization;
	extern TAutoConsoleVariable<bool> CVar_IgnoreInvalidExtensions;

	// Config strings
	extern FString GDataSource_Uncooked;
	extern FString GDataSource_Cooked;

	// Console commands
	extern FAutoConsoleCommand CCommand_ReimportAllAssets;
} // namespace OUU::Runtime::JsonData::Private
