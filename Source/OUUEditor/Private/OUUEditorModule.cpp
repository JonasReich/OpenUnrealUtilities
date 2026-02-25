// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "Editor.h"
#include "Engine/AssetManager.h"
#include "GameEntitlements/OUUGameEntitlements.h"
#include "GameEntitlements/OUUGameEntitlementsSettings.h"
#include "ISinglePropertyView.h"
#include "LevelEditor.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "Modules/ModuleManager.h"
#include "OUUContentBrowserExtensions.h"

namespace OUU::Editor
{
	class FOUUEditorModule : public IModuleInterface
	{
	public:
		void StartupModule() override
		{
			MaterialAnalyzer::RegisterNomadTabSpawner();
			ContentBrowserExtensions::RegisterHooks();

			if (FLevelEditorModule* LevelEditorModule =
					FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
			{
				EntitlementsMenuExtender = MakeShareable(new FExtender());
				EntitlementsMenuExtender->AddToolBarExtension(
					"Play",
					EExtensionHook::After,
					nullptr,
					FToolBarExtensionDelegate::CreateRaw(
						this,
						&FOUUEditorModule::CreateGameEntitlementsToolbarExtension));
				LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(EntitlementsMenuExtender);
			}
		}

		void ShutdownModule() override
		{
			if (OnUtilityWidgetsLoadedHandle.IsValid())
			{
				OnUtilityWidgetsLoadedHandle->CancelHandle();
				OnUtilityWidgetsLoadedHandle = nullptr;
			}

			if (EntitlementsMenuExtender.IsValid())
			{
				if (FLevelEditorModule* LevelEditorModule =
						FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor")))
				{
					LevelEditorModule->GetToolBarExtensibilityManager()->RemoveExtender(EntitlementsMenuExtender);
				}
			}
			EntitlementsMenuExtender.Reset();

			MaterialAnalyzer::UnregisterNomadTabSpawner();
			ContentBrowserExtensions::UnregisterHooks();
		}

	private:
		TSharedPtr<FStreamableHandle> OnUtilityWidgetsLoadedHandle;
		TSharedPtr<FExtender> EntitlementsMenuExtender;

		void CreateGameEntitlementsToolbarExtension(FToolBarBuilder& ToolbarBuilder)
		{
			if (UOUUGameEntitlementSettings::Get().EnablePIEToolbarExtension == false)
			{
				return;
			}

			ToolbarBuilder.BeginSection("OUUEntitlements");
			{
				FPropertyEditorModule& PropertyEditorModule =
					FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
				FSinglePropertyParams PropertyParams;
				PropertyParams.NameOverride = INVTEXT("Entitlement\nOverride\nVersion");
				const TSharedPtr<ISinglePropertyView> OverrideEntitlementProperty =
					PropertyEditorModule.CreateSingleProperty(
						&UOUUGameEntitlementsSubsystem::Get(),
						TEXT("OverrideVersion"),
						PropertyParams);
				if (OverrideEntitlementProperty.IsValid())
				{
					ToolbarBuilder.AddWidget(OverrideEntitlementProperty.ToSharedRef());
				}
			}
			ToolbarBuilder.EndSection();
		}
	};
} // namespace OUU::Editor

IMPLEMENT_MODULE(OUU::Editor::FOUUEditorModule, OUUEditor)
