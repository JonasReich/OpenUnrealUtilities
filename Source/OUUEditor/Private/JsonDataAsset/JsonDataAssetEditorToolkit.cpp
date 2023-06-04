// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonDataAssetEditorToolkit.h"

#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "JsonDataAsset/JsonDataAssetEditor.h"

TSharedRef<FJsonDataAssetEditorToolkit> FJsonDataAssetEditorToolkit::CreateEditor(
	const EToolkitMode::Type Mode,
	const TSharedPtr<IToolkitHost>& InitToolkitHost,
	const TArray<UObject*>& ObjectsToEdit)
{
	TSharedRef<FJsonDataAssetEditorToolkit> NewEditor(new FJsonDataAssetEditorToolkit());
	NewEditor->InitEditor(Mode, InitToolkitHost, ObjectsToEdit, FSimpleAssetEditor::FGetDetailsViewObjects());

	NewEditor->ExtendToolBar();

	return NewEditor;
}

void FJsonDataAssetEditorToolkit::ExtendToolBar()
{
	TSharedRef<FExtender> ToolbarExtender = MakeShared<FExtender>();

	ToolbarExtender->AddToolBarExtension(
		"Asset",
		EExtensionHook::After,
		GetToolkitCommands(),
		FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder& ToolbarBuilder) {
			// #TODO Use command that can be reused in context menu?
			ToolbarBuilder.AddToolBarButton(
				FUIAction(FExecuteAction::CreateLambda([this]() {
					if (auto* ObjectsBeingEdited = this->GetObjectsCurrentlyBeingEdited())
					{
						TArray<FJsonDataAssetPath> Paths;
						for (auto* Object : *ObjectsBeingEdited)
						{
							if (auto* JsonDataAsset = Cast<UJsonDataAsset>(Object))
							{
								Paths.Add(JsonDataAsset->GetPath());
							}
						}
						OUU::Editor::JsonData::ContentBrowser_NavigateToSources(Paths);
					}
				})),
				NAME_None,
				INVTEXT("Browse to Source"),
				INVTEXT("Browses to the source file and selects it in the most recently used Content Browser"),
				FSlateIcon("EditorStyle", "Icons.OpenSourceLocation"));
		}));

	AddToolbarExtender(ToolbarExtender);

	RegenerateMenusAndToolbars();
}
