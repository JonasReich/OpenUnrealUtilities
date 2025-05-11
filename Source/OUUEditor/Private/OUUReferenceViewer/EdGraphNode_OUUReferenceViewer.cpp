// Copyright (c) 2025 Jonas Reich & Contributors

#include "OUUReferenceViewer/EdGraphNode_OUUReferenceViewer.h"

void UEdGraphNode_OUUReferenceViewer::Setup(FText& InNodeTitle, TWeakObjectPtr<UObject> InOptionalPayload)
{
	ReferencerPin = CreatePin(EEdGraphPinDirection::EGPD_Input, NAME_None, NAME_None);
	ReferencerPin->bHidden = true;
	DependencyPin = CreatePin(EEdGraphPinDirection::EGPD_Output, NAME_None, NAME_None);
	DependencyPin->bHidden = true;
	NodeTitle = InNodeTitle;
	PayloadObject = InOptionalPayload;
}

void UEdGraphNode_OUUReferenceViewer::AddReferencer(UEdGraphNode_OUUReferenceViewer& Referencer)
{
	UEdGraphPin* ReferencerDependencyPin = Referencer.DependencyPin;

	if (ensure(ReferencerDependencyPin))
	{
		ReferencerDependencyPin->bHidden = false;
		ReferencerPin->bHidden = false;
		ReferencerPin->MakeLinkTo(ReferencerDependencyPin);
	}
}

TWeakObjectPtr<UObject> UEdGraphNode_OUUReferenceViewer::GetPayload() const
{
	return PayloadObject;
}

FText UEdGraphNode_OUUReferenceViewer::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	return NodeTitle;
}

FLinearColor UEdGraphNode_OUUReferenceViewer::GetNodeTitleColor() const
{
	return FLinearColor::Black;
}
