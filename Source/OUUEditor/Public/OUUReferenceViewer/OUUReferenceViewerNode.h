// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

struct FOUUReferenceViewerNode
{
	FOUUReferenceViewerNode() = default;

	FText NodeTitle;
	TWeakObjectPtr<UObject> OptionalPayload;
	TArray<TUniquePtr<FOUUReferenceViewerNode>> References;
};
