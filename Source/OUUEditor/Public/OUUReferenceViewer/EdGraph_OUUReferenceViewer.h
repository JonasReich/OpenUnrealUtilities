// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EdGraph/EdGraph.h"
#include "OUUReferenceViewer/OUUReferenceViewerNode.h"

#include "EdGraph_OUUReferenceViewer.generated.h"

class UEdGraphNode_OUUReferenceViewer;

UCLASS()
class OUUEDITOR_API UEdGraph_OUUReferenceViewer : public UEdGraph
{
	GENERATED_BODY()

public:
	void RebuildGraph();

	virtual TArray<TUniquePtr<FOUUReferenceViewerNode>> RebuildNodes() { return {}; }

	DECLARE_EVENT(UEdGraph_OUUReferenceViewer, FOnGraphRebuilt)
	FOnGraphRebuilt OnGraphRebuilt;

private:
	void RecursivelyBuildGraph(
		FOUUReferenceViewerNode& Node,
		UEdGraphNode_OUUReferenceViewer* Referencer,
		FIntPoint& InOutCell);
};
