// Copyright (c) 2026 Jonas Reich & Contributors

#include "PIESettings/OUUPIESettingsTab.h"

#include "Framework/Docking/TabManager.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "PIESettings/OUUPIESettingsRegistry.h"
#include "PIESettings/SOUUPIESettingsPanel.h"
#include "Styling/AppStyle.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "WorkspaceMenuStructure.h"
#include "WorkspaceMenuStructureModule.h"

namespace OUU::Editor::Private::PIESettings
{
	using namespace OUU::Editor::PIESettings;

	static const FName GTabName = TEXT("OUUPIESettings");

	static TSharedPtr<FExtender> GToolbarExtender;

	//------------------------------------------------------------------------------------------------------------------
	static TSharedRef<SDockTab> SpawnTab(const FSpawnTabArgs& TabSpawnArgs)
	{
		return SNew(SDockTab).TabRole(ETabRole::NomadTab)[SNew(SPIESettingsPanel)];
	}

	//------------------------------------------------------------------------------------------------------------------
	static FText MakeStatusButtonToolTip()
	{
		TArray<FText> Lines;
		for (const FCapability& Capability : GetCapabilities())
		{
			if (Capability.Evaluate == nullptr)
			{
				continue;
			}

			const FCapabilityState State = Capability.Evaluate();
			if (State.Status != ECapabilityStatus::Available)
			{
				Lines.Add(FText::Format(
					INVTEXT("{0}: {1} - {2}"),
					Capability.DisplayName,
					GetStatusDisplayName(State.Status),
					State.Reason));
			}
		}

		if (Lines.IsEmpty())
		{
			return INVTEXT("PIE settings - everything is testable in your next Play session.");
		}

		Lines.Insert(INVTEXT("PIE settings - your next Play session cannot fully test:"), 0);
		return FText::Join(INVTEXT("\n"), Lines);
	}

	//------------------------------------------------------------------------------------------------------------------
	static void CreateToolbarExtension(FToolBarBuilder& ToolbarBuilder)
	{
		ToolbarBuilder.BeginSection("OUUPIESettings");
		{
			for (const FSettingEntry& Entry : GetSettings())
			{
				if (Entry.bShowInToolbar == false || Entry.MakeWidget == nullptr)
				{
					continue;
				}

				// clang-format off
				const TSharedRef<SWidget> EntryWidget =
					SNew(SBox)
					.VAlign(VAlign_Center)
					.Padding(4.f, 0.f)
					.ToolTipText(Entry.Explanation)
					[
						SNew(SBox)
						.MinDesiredWidth(120.f)
						[
							Entry.MakeWidget()
						]
					];
				// clang-format on

				ToolbarBuilder.AddWidget(EntryWidget);
			}

			// clang-format off
			const TSharedRef<SWidget> StatusButton =
				SNew(SBox)
				.VAlign(VAlign_Center)
				.Padding(4.f, 0.f)
				[
					SNew(SButton)
					.ButtonStyle(FAppStyle::Get(), "SimpleButton")
					.ToolTipText_Static(&MakeStatusButtonToolTip)
					.OnClicked_Lambda([]() { OpenTab(); return FReply::Handled(); })
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush("Icons.Settings"))
						.ColorAndOpacity_Lambda([]() { return GetStatusColor(GetWorstCapabilityStatus()); })
					]
				];
			// clang-format on

			ToolbarBuilder.AddWidget(StatusButton);
		}
		ToolbarBuilder.EndSection();
	}
} // namespace OUU::Editor::Private::PIESettings

namespace OUU::Editor::PIESettings
{
	//------------------------------------------------------------------------------------------------------------------
	void RegisterNomadTabSpawner()
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(
				Private::PIESettings::GTabName,
				FOnSpawnTab::CreateStatic(&Private::PIESettings::SpawnTab))
			.SetDisplayName(INVTEXT("PIE Settings"))
			.SetTooltipText(INVTEXT("Settings that shape the next Play in Editor session, and what they let you test."))
			.SetGroup(WorkspaceMenu::GetMenuStructure().GetDeveloperToolsMiscCategory())
			.SetIcon(FSlateIcon(FAppStyle::GetAppStyleSetName(), "Icons.Settings"));
	}

	//------------------------------------------------------------------------------------------------------------------
	void UnregisterNomadTabSpawner()
	{
		FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(Private::PIESettings::GTabName);
	}

	//------------------------------------------------------------------------------------------------------------------
	void RegisterToolbarExtension()
	{
		FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
		if (LevelEditorModule == nullptr)
		{
			return;
		}

		Private::PIESettings::GToolbarExtender = MakeShared<FExtender>();
		Private::PIESettings::GToolbarExtender->AddToolBarExtension(
			"Play",
			EExtensionHook::After,
			nullptr,
			FToolBarExtensionDelegate::CreateStatic(&Private::PIESettings::CreateToolbarExtension));

		LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(Private::PIESettings::GToolbarExtender);
	}

	//------------------------------------------------------------------------------------------------------------------
	void UnregisterToolbarExtension()
	{
		if (Private::PIESettings::GToolbarExtender.IsValid() == false)
		{
			return;
		}

		if (FLevelEditorModule* LevelEditorModule =
				FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
		{
			LevelEditorModule->GetToolBarExtensibilityManager()->RemoveExtender(Private::PIESettings::GToolbarExtender);
		}

		Private::PIESettings::GToolbarExtender.Reset();
	}

	//------------------------------------------------------------------------------------------------------------------
	void OpenTab() { FGlobalTabmanager::Get()->TryInvokeTab(Private::PIESettings::GTabName); }
} // namespace OUU::Editor::PIESettings
