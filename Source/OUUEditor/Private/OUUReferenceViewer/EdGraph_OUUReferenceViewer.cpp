// Copyright (c) 2025 Jonas Reich & Contributors

#include "OUUReferenceViewer/EdGraph_OUUReferenceViewer.h"

#include "OUUReferenceViewer/EdGraphNode_OUUReferenceViewer.h"
#include "OUUReferenceViewer/OUUReferenceViewerNode.h"

void UEdGraph_OUUReferenceViewer::RebuildGraph()
{
	while (Nodes.IsEmpty() == false)
	{
		RemoveNode(Nodes.Last());
	}

	auto RootNodes = RebuildNodes();
	FIntPoint Cell = FIntPoint::ZeroValue;
	for (auto& RootNode : RootNodes)
	{
		if (RootNode)
		{
			RecursivelyBuildGraph(*RootNode, nullptr, IN OUT Cell);
		}
		Cell.Y += 5;
	}

	OnGraphRebuilt.Broadcast();
}

void UEdGraph_OUUReferenceViewer::RecursivelyBuildGraph(
	FOUUReferenceViewerNode& Node,
	UEdGraphNode_OUUReferenceViewer* Referencer,
	FIntPoint& InOutCell)
{
	TGuardValue<int32> ColumnGuard{InOutCell.X, InOutCell.X + 1};

	auto* pNewNode =
		Cast<UEdGraphNode_OUUReferenceViewer>(CreateNode(UEdGraphNode_OUUReferenceViewer::StaticClass(), false));
	AddNode(pNewNode);
	pNewNode->Setup(Node);

	constexpr int32 ColumnWidth = 500;
	constexpr int32 RowHeight = 100;
	pNewNode->NodePosX = InOutCell.X * ColumnWidth;
	pNewNode->NodePosY = InOutCell.Y * RowHeight;

	if (Referencer)
	{
		pNewNode->AddReferencer(*Referencer);
	}

	for (int32 i = 0; i < Node.References.Num(); ++i)
	{
		if (i > 0)
		{
			InOutCell.Y++;
		}

		auto& ReferenceNode = Node.References[i];
		if (ReferenceNode.IsValid())
		{
			RecursivelyBuildGraph(*ReferenceNode, pNewNode, IN OUT InOutCell);
		}
	}

	// Move the node down to half the height of all reference nodes
	pNewNode->NodePosY = (pNewNode->NodePosY + InOutCell.Y * RowHeight) / 2;
}
