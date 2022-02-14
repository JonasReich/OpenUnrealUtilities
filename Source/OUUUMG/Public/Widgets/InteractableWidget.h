// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Widgets/UserFocusResetableWidget.h"

#include "InteractableWidget.generated.h"

class UUMGInputActionBindingStack;

/**
 * Base widget that shows how you can integrate UUMGInputActionBindingStack and how
 * to implement the IUserFocusResettableWidget interface using the template TUserFocusResetableWidget_Impl<T>.
 */
UCLASS()
class OUUUMG_API UOUUInteractableWidget :
	public UUserWidget,
	public IUserFocusResetableWidget,
	public TUserFocusResetableWidget_Impl<UOUUInteractableWidget>
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
	UUMGInputActionBindingStack* InputActionBindingStack = nullptr;
};
