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

	const auto GraphEditor = SNew(SGraphEditor)
						   .GraphToEdit(OwnedGraph)
						   .GraphEvents(GraphEvents)
						   .IsEditable(false)
						   .ShowGraphStateOverlay(false);

	constexpr bool bZoomToFitOnlySelected = false;
	OwnedGraph->OnGraphRebuilt.AddSP(GraphEditor, &SGraphEditor::ZoomToFit, bZoomToFitOnlySelected);

	FToolBarBuilder ToolBarBuilder(UIActions, FMultiBoxCustomization::None, InArgs._ToolbarExtenders, true);
	ToolBarBuilder.SetStyle(&FAppStyle::Get(), "AssetEditorToolbar");
	ToolBarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateUObject(OwnedGraph, &UEdGraph_OUUReferenceViewer::RebuildGraph)),
		NAME_None,
		TAttribute<FText>(),
		INVTEXT("Rebuild reference graph"),
		FSlateIcon(FAppStyle::Get().GetStyleSetName(), "Icons.Refresh"));
	ToolBarBuilder.BeginSection("Extensions");
	ToolBarBuilder.EndSection();

	// clang-format off
	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight()
		[
			SNew(SBorder)
			.BorderImage(FAppStyle::GetBrush("Brushes.Panel"))
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot().AutoWidth().Padding(4, 0)
				[
					ToolBarBuilder.MakeWidget()
				]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)[GraphEditor]
	];
	// clang-format on
}
