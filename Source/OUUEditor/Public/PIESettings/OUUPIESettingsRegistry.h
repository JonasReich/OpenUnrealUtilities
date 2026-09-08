// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Styling/SlateColor.h"

class SWidget;

namespace OUU::Editor::PIESettings
{
	// Ordered by increasing severity so the worst status across all capabilities is the maximum.
	enum class ECapabilityStatus : uint8
	{
		Available,
		Limited,
		Unavailable
	};

	struct FCapabilityState
	{
		ECapabilityStatus Status = ECapabilityStatus::Available;

		// Shown beside the capability. For anything but Available this should name the setting responsible, because it
		// is the only pointer the reader gets to the row that fixes it.
		FText Reason;
	};

	struct FSettingEntry
	{
		FName Id;
		FText DisplayName;
		FText Explanation;
		FString Group;

		// Also render this entry in the level editor toolbar, not just in the window.
		bool bShowInToolbar = false;

		// Entries are sorted by this within their group; ties keep registration order.
		int32 SortOrder = 0;

		// Set for console variable entries only. It is what SaveConsoleVariablesToUserConfig writes, so it stays
		// authoritative even if a caller gives the entry a different Id.
		FName ConsoleVariableName;

		TFunction<TSharedRef<SWidget>()> MakeWidget;
	};

	struct FCapability
	{
		FName Id;
		FText DisplayName;
		FText Description;
		TFunction<FCapabilityState()> Evaluate;
	};

	// Builds an entry that edits Object's PropertyName through the same widget the details panel would use.
	// The value is written straight to Object and then persisted with SaveConfig, which the property editor does not do
	// on its own outside of the settings panels.
	OUUEDITOR_API FSettingEntry
		MakePropertyEntry(const FString& Group, const FName& Id, UObject* Object, FName PropertyName);

	// Builds an entry for a console variable. Bool, int, float and string variables are supported; the name is resolved
	// when the row is built, and a name that resolves to nothing renders as an error row rather than disappearing.
	// Id and DisplayName are both set to ConsoleVariableName, so a row is always labelled with the name you would type
	// into the console. Overriding DisplayName defeats that and should not be done.
	OUUEDITOR_API FSettingEntry
		MakeConsoleVariableEntry(const FString& Group, const FName& ConsoleVariableName, const FText& Explanation);

	// Builds an entry around an arbitrary widget, for settings whose editor cannot be derived from a property or a
	// console variable.
	OUUEDITOR_API FSettingEntry
		MakeWidgetEntry(const FString& Group, const FName& Id, TFunction<TSharedRef<SWidget>()> WidgetFactory);

	OUUEDITOR_API void RegisterSetting(FSettingEntry Entry);
	OUUEDITOR_API void UnregisterSetting(FName Id);

	OUUEDITOR_API void RegisterCapability(FCapability Capability);
	OUUEDITOR_API void UnregisterCapability(FName Id);

	// Sorted by group, then SortOrder.
	const TArray<FSettingEntry>& GetSettings();
	const TArray<FCapability>& GetCapabilities();

	// Available when nothing is registered.
	OUUEDITOR_API ECapabilityStatus GetWorstCapabilityStatus();

	// Writes the current value of every registered console variable entry into the [ConsoleVariables] section of the
	// local Engine.ini, which the engine applies on startup. Returns how many were written.
	// This pins every listed variable, including ones you never touched, so later changes to their defaults no longer
	// reach this machine until the entry is removed from the ini by hand.
	OUUEDITOR_API int32 SaveConsoleVariablesToUserConfig();

	FText GetStatusDisplayName(ECapabilityStatus Status);
	FSlateColor GetStatusColor(ECapabilityStatus Status);
} // namespace OUU::Editor::PIESettings
