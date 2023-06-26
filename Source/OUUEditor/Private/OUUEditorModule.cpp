// Copyright (c) 2023 Jonas Reich & Contributors

#include "CoreMinimal.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "DetailsCustomizations/JsonDataAssetPathDetailsCustomization.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "GameplayTags/TypedGameplayTag.h"
#include "IAssetTools.h"
#include "Interfaces/IPluginManager.h"
#include "JsonDataAsset/AssetTypeActionsJsonDataAsset.h"
#include "JsonDataAsset/ContentBrowserJsonDataSource.h"
#include "JsonDataAsset/JsonAssetReferenceFilter.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "Modules/ModuleManager.h"
#include "OUUContentBrowserExtensions.h"
#include "PropertyEditorUtils.h"

namespace OUU::Editor
{
	class FOUUEditorModule : public IModuleInterface
	{
	public:
		virtual void StartupModule() override
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

			FCoreDelegates::OnAllModuleLoadingPhasesComplete.AddLambda([]() {
				FTypedGameplayTag_Base::RegisterAllDerivedPropertyTypeLayouts();

#if UE_VERSION_NEWER_THAN(5, 2, 999)
				COMPILE_ERROR(
					"Asset reference filter only implemented for 5.2. Please review this code and check if it's "
					"now possible to bind an asset referencing filter without breaking previously registered "
					"filters like FDomainAssetReferenceFilter.")
#else
				// This is the only plugin in 5.1 that can conflict with our code.
				// Needs to be reviewed for future engine versions!
				auto AssetReferenceRestrictionsPlugin = IPluginManager::Get().FindPlugin("AssetReferenceRestrictions");
				if (AssetReferenceRestrictionsPlugin->IsEnabled())
				{
					UE_LOG(
						LogOpenUnrealUtilities,
						Warning,
						TEXT("AssetReferenceRestrictions plugin is enabled which prevents registering the "
							 "FJsonAssetReferenceFilter!"))
				}
				else
				{
					GEditor->OnMakeAssetReferenceFilter().BindLambda(
						[](const FAssetReferenceFilterContext& Context) -> TSharedPtr<IAssetReferenceFilter> {
							return MakeShared<FJsonAssetReferenceFilter>(Context);
						});
				}
#endif
			});

			ContentBrowserJsonDataSource = MakeUnique<FContentBrowserJsonDataSource>();

			IAssetTools::Get().RegisterAssetTypeActions(MakeShared<FAssetTypeActions_JsonDataAsset>());
		}

		virtual void ShutdownModule() override
		{
			ContentBrowserJsonDataSource.Reset();

			MaterialAnalyzer::UnregisterNomadTabSpawner();
			ContentBrowserExtensions::UnregisterHooks();

			FTypedGameplayTag_Base::UnregisterAllDerivedPropertyTypeLayouts();
		}

	private:
		TUniquePtr<FContentBrowserJsonDataSource> ContentBrowserJsonDataSource;
		FDelegateHandle OnFilesLoadedHandle;

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
		static void RegisterAllEditorUtilityWidgetTabs()
		{
			if (!GEditor)
				return;

			TArray<FAssetData> BlueprintList;
			FARFilter Filter;
			Filter.ClassPaths.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetClassPathName());
			Filter.bRecursiveClasses = true;
			IAssetRegistry::GetChecked().GetAssets(Filter, BlueprintList);

			UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
			if (!IsValid(EditorUtilitySubsystem))
				return;

			for (auto& Asset : BlueprintList)
			{
				if (auto* EditorWidget = Cast<UEditorUtilityWidgetBlueprint>(Asset.GetAsset()))
				{
					FName TabId;
					EditorUtilitySubsystem->RegisterTabAndGetID(EditorWidget, OUT TabId);
				}
			}
		}
	};
} // namespace OUU::Editor

IMPLEMENT_MODULE(OUU::Editor::FOUUEditorModule, OUUEditor)
