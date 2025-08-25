// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"
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
		}

		void ShutdownModule() override
		{
			if (OnUtilityWidgetsLoadedHandle.IsValid())
			{
				OnUtilityWidgetsLoadedHandle->CancelHandle();
				OnUtilityWidgetsLoadedHandle = nullptr;
			}

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
			if (!GEditor)
				return;

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

			OnUtilityWidgetsLoadedHandle =
				StreamableManager.RequestAsyncLoad(AssetPathsToLoad, [this, AssetPathsToLoad]() -> void {
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
				});
		}
	};
} // namespace OUU::Editor

IMPLEMENT_MODULE(OUU::Editor::FOUUEditorModule, OUUEditor)
