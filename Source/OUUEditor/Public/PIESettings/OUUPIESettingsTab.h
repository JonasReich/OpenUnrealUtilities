// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

namespace OUU::Editor::PIESettings
{
	void RegisterNomadTabSpawner();
	void UnregisterNomadTabSpawner();

	// Renders every entry flagged bShowInToolbar, plus the button that opens the tab.
	void RegisterToolbarExtension();
	void UnregisterToolbarExtension();

	OUUEDITOR_API void OpenTab();
} // namespace OUU::Editor::PIESettings
