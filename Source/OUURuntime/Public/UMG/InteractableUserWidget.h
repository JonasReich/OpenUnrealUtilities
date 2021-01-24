// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMG/UserFocusResetableWidget.h"
#include "InteractableUserWidget.generated.h"

class UUMGInputActionBindingStack;

/**
 * Base widget that shows how you can integrate UUMGInputActionBindingStack and how
 * to implement the IUserFocusResettableWidget interface using the template TUserFocusResetableWidget_Impl<T>.
 */
UCLASS()
class OUURUNTIME_API UInteractableUserWidget : public UUserWidget,
	public IUserFocusResetableWidget,
	public TUserFocusResetableWidget_Impl<UInteractableUserWidget>
{
	GENERATED_BODY()
protected:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Input")
	UUMGInputActionBindingStack* GetInputActionBindingStack();

	// - UUserWidget
	FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	// --

	// - IUserFocusResetableWidget
	bool ResetUserFocus_Implementation() override;
	// --
private:
	/** Input action binding stack. To be lazily created by GetInputActionBindingStack() function. */
	UPROPERTY(Transient)
	UUMGInputActionBindingStack* InputActionBindingStack;
};
