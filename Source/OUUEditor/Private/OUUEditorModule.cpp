// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"
#include "GameEntitlements/OUUGameEntitlements.h"
#include "GameEntitlements/OUUGameEntitlementsSettings.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "Modules/ModuleManager.h"
#include "OUUContentBrowserExtensions.h"
#include "PIESettings/OUUPIESettingsRegistry.h"
#include "PIESettings/OUUPIESettingsTab.h"

namespace OUU::Editor
{
	static const FName GEntitlementOverrideVersionId = TEXT("OUU.Entitlements.OverrideVersion");
	// The console variable entry is identified by the variable name itself.
	static const FName GEntitlementUnlockAllDlcId = TEXT("ouu.Entitlements.UnlockAllDlc");

	class FOUUEditorModule : public IModuleInterface
	{
	public:
		void StartupModule() override
		{
			IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
			if (AssetRegistry.IsLoadingAssets())
			{
				OnFilesLoadedHandle =
					AssetRegistry.OnFilesLoaded().AddRaw(this, &FOUUEditorModule::HandleOnFiledLoaded);
			}
			else
			{
				RegisterAllEditorUtilityWidgetTabs();
			}

			MaterialAnalyzer::RegisterNomadTabSpawner();
			ContentBrowserExtensions::RegisterHooks();

			FCoreDelegates::OnPostEngineInit.AddRaw(this, &FOUUEditorModule::RegisterGameEntitlementsPIESettings);

			PIESettings::RegisterNomadTabSpawner();
			PIESettings::RegisterToolbarExtension();
		}

		void ShutdownModule() override
		{
			if (OnUtilityWidgetsLoadedHandle.IsValid())
			{
				OnUtilityWidgetsLoadedHandle->CancelHandle();
				OnUtilityWidgetsLoadedHandle = nullptr;
			}

			FCoreDelegates::OnPostEngineInit.RemoveAll(this);

			PIESettings::UnregisterToolbarExtension();
			PIESettings::UnregisterNomadTabSpawner();
			UnregisterGameEntitlementsPIESettings();

			MaterialAnalyzer::UnregisterNomadTabSpawner();
			ContentBrowserExtensions::UnregisterHooks();
		}

	private:
		FDelegateHandle OnFilesLoadedHandle;
		TSharedPtr<FStreamableHandle> OnUtilityWidgetsLoadedHandle;

		void HandleOnFiledLoaded()
		{
			IAssetRegistry::GetChecked().OnFilesLoaded().Remove(OnFilesLoadedHandle);
			OnFilesLoadedHandle.Reset();
			RegisterAllEditorUtilityWidgetTabs();
		}

		/**
		 * Search and register all editor utility widget blueprints so they can be opened from the "Developer Tools"
		 * menu.
		 */
		void RegisterAllEditorUtilityWidgetTabs()
		{
			if (GIsEditor == false || IsRunningCommandlet())
			{
				return;
			}

			TArray<FAssetData> BlueprintList;
			FARFilter Filter;
			Filter.ClassPaths.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetClassPathName());
			Filter.bRecursiveClasses = true;
			IAssetRegistry::GetChecked().GetAssets(Filter, BlueprintList);

			if (BlueprintList.IsEmpty())
				return;

			TArray<FSoftObjectPath> AssetPathsToLoad;
			AssetPathsToLoad.Reserve(BlueprintList.Num());
			for (const auto& AssetData : BlueprintList)
			{
				if (bool bRunOnStartup = false;
					AssetData.GetTagValue<bool>(TEXT("bRunEditorUtilityOnStartup"), bRunOnStartup) && bRunOnStartup)
				{
					AssetPathsToLoad.Add(AssetData.GetSoftObjectPath());
				}
			}

			FStreamableManager& StreamableManager = UAssetManager::Get().GetStreamableManager();

			OnUtilityWidgetsLoadedHandle = StreamableManager.RequestAsyncLoad(
				AssetPathsToLoad,
				[this, AssetPathsToLoad]() -> void {
					UEditorUtilitySubsystem* EditorUtilitySubsystem =
						GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();

					if (!IsValid(EditorUtilitySubsystem))
						return;

					for (const auto& AssetPath : AssetPathsToLoad)
					{
						if (auto* EditorWidgetBlueprint =
								Cast<UEditorUtilityWidgetBlueprint>(AssetPath.ResolveObject()))
						{
							if (EditorWidgetBlueprint->GeneratedClass)
							{
								const UEditorUtilityWidget* EditorUtilityWidget =
									EditorWidgetBlueprint->GeneratedClass->GetDefaultObject<UEditorUtilityWidget>();
								if (EditorUtilityWidget && EditorUtilityWidget->ShouldAlwaysReregisterWithWindowsMenu())

								{
									FName TabId;
									EditorUtilitySubsystem->RegisterTabAndGetID(EditorWidgetBlueprint, OUT TabId);
								}
							}
						}
					}

					OnUtilityWidgetsLoadedHandle = nullptr;
				},
				FStreamableManager::DefaultAsyncLoadPriority,
				false,
				false,
				TEXT("RegisterEditorUtilityWidgets"));
		}

		PIESettings::FCapabilityState EvaluateEntitledContent()
		{
			auto& EntitlementSubsystem = UOUUGameEntitlementsSubsystem::Get();
			const FOUUGameEntitlementVersion ActiveVersion = EntitlementSubsystem.GetActiveVersion();
			if (ActiveVersion.IsValid() == false)
			{
				return PIESettings::FCapabilityState(
					PIESettings::ECapabilityStatus::Unavailable,
					INVTEXT("No entitlement version resolved, so every gated module is locked."));
			}

			for (auto& DLCEntry : UOUUGameEntitlementSettings::Get().SteamDlcEntitlements)
			{
				for (auto& Tag : DLCEntry.Value)
				{
					if (EntitlementSubsystem.IsEntitled(FOUUGameEntitlementModule::ConvertChecked(Tag)) == false)
					{
						return PIESettings::FCapabilityState(
							PIESettings::ECapabilityStatus::Limited,
							FText::Format(
								INVTEXT("no entitlement for DLC {0}"),
								FText::FromName(Tag.GetTagLeafName())));
					}
				}
			}

			return PIESettings::FCapabilityState(
				PIESettings::ECapabilityStatus::Available,
				FText::Format(INVTEXT("Running as '{0}'."), FText::FromName(ActiveVersion.GetTagLeafName())));
		}

		void RegisterGameEntitlementsPIESettings()
		{
			PIESettings::FSettingEntry OverrideVersion = PIESettings::MakePropertyEntry(
				TEXT("Entitlements"),
				GEntitlementOverrideVersionId,
				&UOUUGameEntitlementsSubsystem::Get(),
				TEXT("OverrideVersion"));
			OverrideVersion.bShowInToolbar = true;
			PIESettings::RegisterSetting(MoveTemp(OverrideVersion));

			PIESettings::FCapability EntitlementCapability{
				TEXT("TQ2.Capability.EntitledContent"),
				INVTEXT("Entitlement-gated content"),
				INVTEXT("Which chapters, data layers and modules the session may reach."),
				[this] { return EvaluateEntitledContent(); }};

			PIESettings::RegisterCapability(MoveTemp(EntitlementCapability));
		}

		void UnregisterGameEntitlementsPIESettings()
		{
			PIESettings::UnregisterSetting(GEntitlementOverrideVersionId);
			PIESettings::UnregisterSetting(GEntitlementUnlockAllDlcId);
		}
	};
} // namespace OUU::Editor

IMPLEMENT_MODULE(OUU::Editor::FOUUEditorModule, OUUEditor)
