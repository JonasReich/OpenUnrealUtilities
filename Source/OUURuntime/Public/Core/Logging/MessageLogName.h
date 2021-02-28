// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "MessageLogName.generated.h"

/**
 * Enum aliases for message log names.
 * The log names are log categories that get individual message streams and editor tabs.
 * Can be converted to FNames of message logs with MessageLogBlueprintLibrary.
 * You can of course always create custom categories - this enum only contains the existing Engine message logs.
 */
UENUM(BlueprintType)
enum class EMessageLogName : uint8
{
	// ----
	// GAME
	// ----
	// Miscellaneous logs to display during Play in Editor gameplay. Comparable to "LogTemp" 
	PIE,

	// ----
	// EDITOR
	// ----
	// Use for uncategorized editor errors.
	EditorErrors,
	// Logging for tools that create/modify assets
	AssetTools,
	// Actor/ActorComponent map check implementable via CheckForErrors()
	MapCheck,
	// Asset check/validation
	AssetCheck,
	// Asset (re)import  
	AssetReimport,

	// ----
	// Special engine subsystem logs.
	// Only use these if you actually extend the functionality of those subsystems.
	// Mainly contained here for completeness.
	// ----
	// Special editor logs
	AnimBlueprintLog,
	// Automation testing. Visible in session frontend.
	// All logs during text execution will be automatically mirrored here.
	AutomationTestingLog,
	// Level build errors (lighting, navigation, etc)
	BuildAndSubmitErrors,
	// Blueprint editor + compiler
	Blueprint,
	BlueprintLog,
	// HLOD generation
	HLODResults,
	// Lighting/lightmap generation
	LightingResults,
	// Asset loading
	LoadErrors,
	// Localization dashboard
	LocalizationService,
	// Project packaging after build 
	PackagingResults,
	// Internal slate messages
	SlateStyleLog,
	// Source control plugins (git, perforce, etc)
	SourceControl,
	// Translation editing
	TranslationEditor,
	// UDN documentation parser
	UDNParser,
	// UMG + Slate widget events displayed in the widget reflector
	WidgetEvents
};
