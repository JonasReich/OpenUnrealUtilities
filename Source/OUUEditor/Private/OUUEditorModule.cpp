// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/AssetManager.h"
#include "GameplayTags/TypedGameplayTag.h"
#include "GameplayTags/TypedGameplayTagContainerCustomization.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "Modules/ModuleManager.h"
#include "OUUContentBrowserExtensions.h"
#include "PropertyEditorUtils.h"
#include "OUUEditor/Public/PropertyEditorUtils.h"

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

			FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda(
				[]() { FTypedGameplayTag_Base::RegisterAllDerivedPropertyTypeLayouts(); });

			OUU::Editor::PropertyEditorUtils::RegisterCustomPropertyTypeLayout<
				FTypedGameplayTagContainer,
				FTypedGameplayTagContainer_PropertyTypeCustomization>();
		}

		void ShutdownModule()
		{
			if (OnUtilityWidgetsLoadedHandle.IsValid())
			{
				OnUtilityWidgetsLoadedHandle->CancelHandle();
				OnUtilityWidgetsLoadedHandle = nullptr;
			}

			MaterialAnalyzer::UnregisterNomadTabSpawner();
			ContentBrowserExtensions::UnregisterHooks();

			FTypedGameplayTag_Base::UnregisterAllDerivedPropertyTypeLayouts();

			OUU::Editor::PropertyEditorUtils::UnregisterCustomPropertyTypeLayout<FTypedGameplayTagContainer>();
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
						if (auto* EditorWidget = Cast<UEditorUtilityWidgetBlueprint>(AssetPath.ResolveObject()))
						{
							FName TabId;
							EditorUtilitySubsystem->RegisterTabAndGetID(EditorWidget, OUT TabId);
						}
					}

					OnUtilityWidgetsLoadedHandle = nullptr;
				});
		}
	};
} // namespace OUU::Editor

IMPLEMENT_MODULE(OUU::Editor::FOUUEditorModule, OUUEditor)
