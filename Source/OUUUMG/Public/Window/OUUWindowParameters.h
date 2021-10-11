// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Window/WindowSizingRule.h"
#include "Window/WindowAutoCenterRule.h"

#include "OUUWindowParameters.generated.h"

/** Initialization parameters for OUUWindows */
USTRUCT(BlueprintType)
struct FOUUWindowParameters
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWindowAutoCenterRule AutoCenterRule = EWindowAutoCenterRule::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsInitiallyMaximized = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCreateTitleBar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWindowSizingRule SizingRule = EWindowSizingRule::Autosized;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSupportsMaximize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSupportsMinimize = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bHasCloseButton = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D ClientSize = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bUseOSWindowBorder = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Title = INVTEXT("");
};

