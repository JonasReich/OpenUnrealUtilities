// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EdGraph_OUUReferenceViewer.h"
#include "Widgets/SCompoundWidget.h"

class OUUEDITOR_API SOUUReferenceViewer : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SOUUReferenceViewer)
		{
		}
		SLATE_ARGUMENT(TSharedPtr<class FExtender>, ToolbarExtenders)
	SLATE_END_ARGS()

	~SOUUReferenceViewer();

	void Construct(const FArguments& InArgs, TSubclassOf<UEdGraph_OUUReferenceViewer> GraphClass);

	template <class ResultClass = UEdGraph_OUUReferenceViewer>
	ResultClass* GetOwnedEdGraph() const
	{
		return Cast<ResultClass>(OwnedGraph);
	}

private:
	UEdGraph_OUUReferenceViewer* OwnedGraph = nullptr;
	TSharedPtr<FUICommandList> UIActions;
};
