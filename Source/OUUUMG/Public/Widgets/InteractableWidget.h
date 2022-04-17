// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"
#include "Widgets/UserFocusResetableWidget.h"

#include "InteractableWidget.generated.h"

class UUMGInputActionBindingStack_DEPRECATED;

PRAGMA_DISABLE_DEPRECATION_WARNINGS

/**
 * Base widget that shows how you can integrate UUMGInputActionBindingStack and how
 * to implement the IUserFocusResettableWidget interface using the template TUserFocusResetableWidget_Impl<T>.
 */
UCLASS()
class OUUUMG_API UOUUInteractableWidget_DEPRECATED :
	public UUserWidget,
	public IUserFocusResetableWidget,
	public TUserFocusResetableWidget_Impl<UOUUInteractableWidget_DEPRECATED>
{
	GENERATED_BODY()
protected:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Input")
	UUMGInputActionBindingStack_DEPRECATED* GetInputActionBindingStack();

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
	UUMGInputActionBindingStack_DEPRECATED* InputActionBindingStack = nullptr;
};

PRAGMA_ENABLE_DEPRECATION_WARNINGS

class UE_DEPRECATED(5.0, "UOUUInteractableWidget is deprecated in favor of UE5's UCommonActivatableWidget.")
	UOUUInteractableWidget;
UCLASS()
class OUUUMG_API UOUUInteractableWidget : public UOUUInteractableWidget_DEPRECATED
{
	GENERATED_BODY()
};
