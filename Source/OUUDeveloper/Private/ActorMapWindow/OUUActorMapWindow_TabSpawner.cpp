// Copyright (c) 2026 Jonas Reich & Contributors

#include "ActorMapWindow/OUUActorMapWindow_TabSpawner.h"

#include "ActorMapWindow/SActorMap.h"
#include "Brushes/SlateColorBrush.h"
#include "Engine/Engine.h"
#include "Framework/Docking/TabManager.h"
#include "GameFramework/PlayerController.h"
#include "TextureResource.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/Layout/SSplitter.h"
#include "Widgets/SWindow.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

#if WITH_EDITOR
	#include "WorkspaceMenuStructure.h"
	#include "WorkspaceMenuStructureModule.h"
#endif

namespace OUU::Developer::ActorMapWindow
{
	TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& Args)
	{
		// clang-format off
			return SNew(SDockTab)
			.Label(ActorMapWindow::GTabTitle)
			[
				SNew(SActorMap)
			];
		// clang-format on
	}

	FName GTabName = TEXT("OUUActorMap");
	FText GTabTitle = INVTEXT("OUU Actor Map");

	void RegisterNomadTabSpawner()
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(GTabName, FOnSpawnTab::CreateStatic(&SpawnTab))
			.SetDisplayName(GTabTitle)
			.SetTooltipText(
				INVTEXT("View a top-down overview of actors in a level (editor or runtime) for debugging purposes."))
#if WITH_EDITOR
			// Previously this was listed as debug tool, now it's in WP because that's easier for level designers to find (by chance)
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetLevelEditorWorldPartitionCategory())
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "ShowFlagsMenu.Grid"))
#endif
			;
	}

	void UnregisterNomadTabSpawner() { FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(GTabName); }

	void TryInvokeTab() { FGlobalTabmanager::Get()->TryInvokeTab(GTabName); }

	//------------------------------------------------------------------------
	// Console command
	//------------------------------------------------------------------------

	static FAutoConsoleCommand OpenActorMapCommand(
		TEXT("ouu.Debug.OpenActorMap"),
		TEXT("Open an actor map for the current world (game or editor)"),
		FConsoleCommandDelegate::CreateStatic(TryInvokeTab));

} // namespace OUU::Developer::ActorMapWindow
