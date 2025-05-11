// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EdGraph/EdGraphNode.h"

#include "EdGraphNode_OUUReferenceViewer.generated.h"

UCLASS()
class OUUEDITOR_API UEdGraphNode_OUUReferenceViewer : public UEdGraphNode
{
	GENERATED_BODY()

public:
	void Setup(FText& NodeTitle, TWeakObjectPtr<UObject> OptionalPayload);
	void AddReferencer(UEdGraphNode_OUUReferenceViewer& Referencer);

	TWeakObjectPtr<UObject> GetPayload() const;

	// - UEdGraphNode interface
public:
	FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	FLinearColor GetNodeTitleColor() const override;

private:
	UEdGraphPin* DependencyPin = nullptr;
	UEdGraphPin* ReferencerPin = nullptr;
	FText NodeTitle;

	TWeakObjectPtr<UObject> PayloadObject;
};
