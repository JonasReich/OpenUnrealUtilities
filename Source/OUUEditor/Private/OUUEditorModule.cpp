// Copyright (c) 2022 Jonas Reich

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

#if UE_VERSION_NEWER_THAN(5, 1, 9999)
				COMPILE_ERROR(
					"Asset reference filter only implemented for 5.1. Please review this code and check if it's "
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
					// #TODO Fix this filter. It would be amazing to have, but right now it if flaky at best,
					// both filtering too much or too little, depending on context.
					// We should probably prevent all asset refs at this stage and use the json file representation for
					// a custom selection widget.

					// That would require at least:
					// - FAR filter based on uassets + converting results to json paths for dropdown entries
					// - drag/drop for content browser items

					GEditor->OnMakeAssetReferenceFilter().BindLambda(
						[](const FAssetReferenceFilterContext& Context) -> TSharedPtr<IAssetReferenceFilter> {
							return MakeShared<FJsonAssetReferenceFilter>(Context);
						});
				}
#endif
			});

			OUU::Editor::PropertyEditorUtils::
				RegisterCustomPropertyTypeLayout<FJsonDataAssetPath, FJsonDataAssetPathCustomization>();

			ContentBrowserJsonDataSource = MakeUnique<FContentBrowserJsonDataSource>();

			IAssetTools::Get().RegisterAssetTypeActions(MakeShared<FAssetTypeActions_JsonDataAsset>());
		}

		virtual void ShutdownModule() override
		{
			ContentBrowserJsonDataSource.Reset();

			MaterialAnalyzer::UnregisterNomadTabSpawner();
			ContentBrowserExtensions::UnregisterHooks();

			OUU::Editor::PropertyEditorUtils::UnregisterCustomPropertyTypeLayout<FJsonDataAssetPath>();

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
