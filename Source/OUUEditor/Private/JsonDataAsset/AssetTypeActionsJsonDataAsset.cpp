// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/AssetTypeActionsJsonDataAsset.h"

#include "JsonDataAsset/JsonDataAssetEditor.h"
#include "ToolMenuSection.h"

void FAssetTypeActions_JsonDataAsset::GetActions(const TArray<UObject*>& InObjects, FToolMenuSection& Section)
{
	TArray<TWeakObjectPtr<UJsonDataAsset>> DataAssets = GetTypedWeakObjectPtrs<UJsonDataAsset>(InObjects);

	Section.AddMenuEntry(
		"JsonDataAsset_NavigateToSource",
		INVTEXT("Show source"),
		INVTEXT("Navigate to the source file in the content browser."),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([DataAssets]() {
			if (DataAssets.Num() > 0 && DataAssets[0].IsValid())
			{
				OUU::Editor::JsonData::ContentBrowser_NavigateToSource(DataAssets[0]->GetPath());
			}
		})));
}
