// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"
#include "GameEntitlements/OUUGameEntitlementsSettings.h"
#include "Gameentitlements/OUUGameEntitlements.h"
#include "GameplayTagsEditorModule.h"
#include "GameplayTagsModule.h"
#include "ISinglePropertyView.h"
#include "LevelEditor.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "Modules/ModuleManager.h"
#include "OUUContentBrowserExtensions.h"
#include "SGameplayTagWidget.h"

namespace OUU::Editor
{
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
		FDelegateHandle OnFilesLoadedHandle;
		TSharedPtr<FStreamableHandle> OnUtilityWidgetsLoadedHandle;
		TSharedPtr<FExtender> EntitlementsMenuExtender;

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
				AssetPathsToLoad.Add(AssetData.GetSoftObjectPath());
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
