// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

struct FOUUReferenceViewerNode
{
	FOUUReferenceViewerNode() = default;

	FText NodeTitle;
	FLinearColor NodeColor = FLinearColor::Black;
	TWeakObjectPtr<UObject> OptionalPayload;
	TArray<TUniquePtr<FOUUReferenceViewerNode>> References;
};
