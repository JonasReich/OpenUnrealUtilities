// Copyright (c) 2022 Jonas Reich

#include "CoreMinimal.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Editor.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "Modules/ModuleManager.h"

class FOUUEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
		IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
		if (AssetRegistry.IsLoadingAssets())
		{
			OnFilesLoadedHandle = AssetRegistry.OnFilesLoaded().AddRaw(this, &FOUUEditorModule::HandleOnFiledLoaded);
		}
		else
		{
			RegisterAllEditorUtilityWidgetTabs();
		}
	}

private:
	FDelegateHandle OnFilesLoadedHandle;

	void HandleOnFiledLoaded()
	{
		IAssetRegistry::GetChecked().OnFilesLoaded().Remove(OnFilesLoadedHandle);
		OnFilesLoadedHandle.Reset();
		RegisterAllEditorUtilityWidgetTabs();
	}

	/**
	 * Search and register all editor utility widget blueprints so they can be opened from the "Developer Tools" menu.
	 */
	static void RegisterAllEditorUtilityWidgetTabs()
	{
		TArray<FAssetData> BlueprintList;
		FARFilter Filter;
		Filter.ClassNames.Add(UEditorUtilityWidgetBlueprint::StaticClass()->GetFName());
		Filter.bRecursiveClasses = true;
		IAssetRegistry::GetChecked().GetAssets(Filter, BlueprintList);

		UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
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

IMPLEMENT_MODULE(FOUUEditorModule, OUUEditor)
