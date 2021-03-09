// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMGInputBinding.h"
#include "UMGInputBindingTests.generated.h"

class UUMGInputActionBindingStack;

UCLASS(meta = (Hidden, HideDropDown))
class UUMGInputBindingTestWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UPROPERTY()
	UUMGInputActionBindingStack* Stack;

	bool bLastNativeOnKeyDownWasHandled = false;
	bool bLastNativeOnKeyUpWasHandled = false;

	int32 HandleInputActionEventOne_NumCalls = 0;
	EUMGInputActionKeyEvent HandleInputActionEventOne_SourceEvent = EUMGInputActionKeyEvent::Any;

	UFUNCTION()
	void HandleInputActionEventOne(EUMGInputActionKeyEvent SourceEvent);

	int32 HandleInputActionEventTwo_NumCalls = 0;
	EUMGInputActionKeyEvent HandleInputActionEventTwo_SourceEvent = EUMGInputActionKeyEvent::Any;

	UFUNCTION()
	void HandleInputActionEventTwo(EUMGInputActionKeyEvent SourceEvent);
	
	// This one does nothing
	UFUNCTION()
	void HandleInputActionEventThree(EUMGInputActionKeyEvent SourceEvent);

	FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
};
