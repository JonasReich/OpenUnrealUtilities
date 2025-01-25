// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Window/WindowAutoCenterRule.h"
#include "Window/WindowSizingRule.h"

#include "OUUWindowParameters.generated.h"

/** Initialization parameters for OUUWindows */
USTRUCT(BlueprintType)
struct FOUUWindowParameters
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	EWindowAutoCenterRule AutoCenterRule = EWindowAutoCenterRule::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	bool bIsInitiallyMaximized = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	bool bCreateTitleBar = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	EWindowSizingRule SizingRule = EWindowSizingRule::Autosized;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	bool bSupportsMaximize = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	bool bSupportsMinimize = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	bool bHasCloseButton = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	FVector2D ClientSize = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	bool bUseOSWindowBorder = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite,Category="OUU|UMG|Window")
	FText Title = INVTEXT("");
};
