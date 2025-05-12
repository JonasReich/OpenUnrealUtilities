// Copyright (c) 2025 Jonas Reich & Contributors

#include "OUUReferenceViewer/SOUUReferenceViewer.h"

#include "OUUReferenceViewer/EdGraphNode_OUUReferenceViewer.h"
#include "OUUReferenceViewer/EdGraph_OUUReferenceViewer.h"

SOUUReferenceViewer::~SOUUReferenceViewer()
{
	if (GExitPurge == false)
	{
		if (ensure(OwnedGraph))
		{
			OwnedGraph->RemoveFromRoot();
		}
	}
}

void SOUUReferenceViewer::Construct(const FArguments& InArgs, TSubclassOf<UEdGraph_OUUReferenceViewer> GraphClass)
{
	OwnedGraph = NewObject<UEdGraph_OUUReferenceViewer>(GetTransientPackage(), GraphClass);
	OwnedGraph->AddToRoot();
	OwnedGraph->Schema = UEdGraphSchema::StaticClass();
	OwnedGraph->RebuildGraph();

	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnNodeDoubleClicked = FSingleNodeEvent::CreateLambda([](UEdGraphNode* _pNode) {
		if (const auto* ReferenceViewerNode = Cast<UEdGraphNode_OUUReferenceViewer>(_pNode))
		{
			if (auto* Payload = ReferenceViewerNode->GetPayload().Get())
			{
				// Default double click implementation assuming the nodes represent assets.
				if (Payload->IsAsset())
				{
					GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Payload);
				}
			}
		}
	});

	auto GraphEditor = SNew(SGraphEditor)
						   .GraphToEdit(OwnedGraph)
						   .GraphEvents(GraphEvents)
						   .IsEditable(false)
						   .ShowGraphStateOverlay(false);

	constexpr bool bZoomToFitOnlySelected = false;
	OwnedGraph->OnGraphRebuilt.AddSP(GraphEditor, &SGraphEditor::ZoomToFit, bZoomToFitOnlySelected);

	ChildSlot[GraphEditor];
}
