// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUEditorLibrary.h"

#include "BlueprintEditor.h"
#include "ContentBrowserModule.h"
#include "GameFramework/Actor.h"
#include "IContentBrowserSingleton.h"
#include "ISessionFrontendModule.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"

void UOUUEditorLibrary::InvokeSessionFrontend(FName Panel)
{
	ISessionFrontendModule& SessionFrontend =
		FModuleManager::LoadModuleChecked<ISessionFrontendModule>("SessionFrontend");
	SessionFrontend.InvokeSessionFrontend(Panel);
}

void UOUUEditorLibrary::RerunConstructionScripts(AActor* Actor)
{
	if (IsValid(Actor))
	{
		Actor->RerunConstructionScripts();
	}
}

void UOUUEditorLibrary::FocusOnBlueprintContent(const FOUUBlueprintEditorFocusContent& FocusContent)
{
	// Copied from STutorialOverlay but reusable globally.
	// -> Step #1: STutorialOverlay::OpenBrowserForWidgetAnchor
	// -> Step #2: STutorialOverlay::FocusOnAnyBlueprintNodes

	auto* EditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (!EditorSubsystem)
		return;

	if (FocusContent.ObjectName.IsEmpty())
		return;

	// -- OpenBrowserForWidgetAnchor --
	IAssetEditorInstance* AssetEditor = nullptr;

	// Check to see if we can find a blueprint relevant to this node and open the editor for that (Then try to get
	// the tabmanager from that)

	// Remove the prefix from the name
	const int32 SpaceIdx = FocusContent.ObjectName.Find(TEXT(" "));
	const FString Name = FocusContent.ObjectName.RightChop(SpaceIdx + 1);

	{
		// Open asset editor
		TArray<FString> AssetPaths;
		AssetPaths.Add(Name);
		EditorSubsystem->OpenEditorsForAssets(AssetPaths);
	}

	UBlueprint* Blueprint = FindFirstObject<UBlueprint>(*Name, EFindFirstObjectOptions::EnsureIfAmbiguous);
	UObject* AssetObject = Blueprint;

	// If we found a blueprint
	if (Blueprint != nullptr)
	{
		IAssetEditorInstance* PotentialAssetEditor = EditorSubsystem->FindEditorForAsset(Blueprint, false);
		if (PotentialAssetEditor != nullptr)
		{
			AssetEditor = PotentialAssetEditor;
		}
	}

	// If we haven't found a tab manager, next check the asset editor that we reference in this tutorial, if any
	if (AssetEditor == nullptr)
	{
		AssetObject = FSoftObjectPath(FocusContent.ObjectName).ResolveObject();
		if (AssetObject != nullptr)
		{
			IAssetEditorInstance* PotentialAssetEditor = EditorSubsystem->FindEditorForAsset(AssetObject, false);
			if (PotentialAssetEditor != nullptr)
			{
				AssetEditor = PotentialAssetEditor;
			}
		}
	}

	if (AssetEditor != nullptr)
	{
		AssetEditor->FocusWindow(AssetObject);
	}

	if (!FocusContent.TabToFocusOrOpen.IsEmpty())
	{
		// Invoke any tab
		if (AssetEditor != nullptr)
		{
			AssetEditor->InvokeTab(FTabId(*FocusContent.TabToFocusOrOpen));
		}
		else
		{
			// fallback to trying the main level editor tab manager
			const FLevelEditorModule& LevelEditorModule =
				FModuleManager::GetModuleChecked<FLevelEditorModule>(TEXT("LevelEditor"));
			const TSharedPtr<FTabManager> LevelEditorTabManager = LevelEditorModule.GetLevelEditorTabManager();
			LevelEditorTabManager->TryInvokeTab(FName(*FocusContent.TabToFocusOrOpen));
		}
	}

	// -- FocusOnAnyBlueprintNodes --

	// If we find a blueprint
	if (Blueprint != nullptr)
	{
		// Try to grab guid
		if (const UEdGraphNode* GraphNode = FBlueprintEditorUtils::GetNodeByGUID(Blueprint, FGuid(FocusContent.NodeGUID)))
		{
			FKismetEditorUtilities::BringKismetToFocusAttentionOnObject(GraphNode, false);
		}
	}
	else if (AssetObject != nullptr)
	{
		const TArray<UObject*> Objects{AssetObject};
		GEditor->SyncBrowserToObjects(Objects);
	}
}

FGuid UOUUEditorLibrary::GetCurrentlySelectedBlueprintNodeGuid()
{
	const FContentBrowserModule& ContentBrowserModule =
		FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	TArray<UClass*> Result;
	for (FAssetData& AssetData : SelectedAssets)
	{
		const UClass* AssetClass = AssetData.GetClass();
		if (AssetClass->IsChildOf<UBlueprint>())
		{
			if (const UBlueprint* Blueprint = Cast<UBlueprint>(AssetData.GetAsset()))
			{
				TSharedPtr<FBlueprintEditor> BlueprintEditor = StaticCastSharedPtr<FBlueprintEditor>(
					FKismetEditorUtilities::GetIBlueprintEditorForObject(Blueprint, false));
				if (BlueprintEditor.IsValid())
				{
					if (const UEdGraphNode* EdGraphNode = BlueprintEditor->GetSingleSelectedNode())
					{
						return EdGraphNode->NodeGuid;
					}
				}
			}
		}
	}

	return FGuid();
}
