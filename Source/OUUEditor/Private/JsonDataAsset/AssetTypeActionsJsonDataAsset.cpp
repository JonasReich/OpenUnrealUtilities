// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/AssetTypeActionsJsonDataAsset.h"

#include "JsonDataAsset/JsonDataAssetEditor.h"
#include "JsonDataAsset/JsonDataAssetEditorToolkit.h"
#include "ToolMenuSection.h"

void FAssetTypeActions_JsonDataAsset::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<UJsonDataAsset>> DataAssets = GetTypedWeakObjectPtrs<UJsonDataAsset>(InObjects);

	Section.AddMenuEntry(
		"JsonDataAsset_NavigateToSource",
		INVTEXT("Browse to Source"),
		INVTEXT("Browses to the source file and selects it in the most recently used Content Browser"),
		FSlateIcon("EditorStyle", "Icons.OpenSourceLocation"),
		FUIAction(FExecuteAction::CreateLambda([DataAssets]() {
			if (DataAssets.Num() > 0)
			{
				TArray<FJsonDataAssetPath> Paths;
				for (auto& Asset : DataAssets)
				{
					if (Asset.IsValid())
					{
						Paths.Add(Asset->GetPath());
					}
				}

				OUU::Editor::JsonData::ContentBrowser_NavigateToSources(Paths);
			}
		})));
}

void FAssetTypeActions_JsonDataAsset::OpenAssetEditor(
	const TArray<UObject*>& InObjects,
	TSharedPtr<IToolkitHost> EditWithinLevelEditor)
{
	FJsonDataAssetEditorToolkit::CreateEditor(EToolkitMode::Standalone, EditWithinLevelEditor, InObjects);
}
