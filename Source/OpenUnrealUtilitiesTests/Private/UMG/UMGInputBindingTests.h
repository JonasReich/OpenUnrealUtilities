// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMGInputBindingTests.generated.h"

UCLASS()
class UUMGInputBindingTestWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION()
	void HandleInputActionEvent(EUMGInputActionKeyEvent SourceEvent)
	{

	}

	UFUNCTION()
	void HandleInputActionEventTwo(EUMGInputActionKeyEvent SourceEvent)
	{

	}
};
