// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UserFocusResetableWidget.h"
#include "ActivatableWidget.generated.h"

class UUMGInputActionBindingStack;

/**
 * Activatable widgets manage a widgets visibility and interactability based on an activation state.
 * They also provide access to an InputActionBindingStack and user focus reset functions that simplify the
 * creation of user widgets that handle input and focus events for gamepad interactable widgets.
 */
UCLASS()
class OUUUMG_API UOUUActivatableWidget : public UUserWidget,
	public IUserFocusResetableWidget,
	public TUserFocusResetableWidget_Impl<UOUUActivatableWidget>
{
	GENERATED_BODY()
public:
	/**
	 * If the widget shall be activated immediately after adding it to the player screen.
	 * Enabling this leads to activation events being called during NativeOnInitialize().
	 */
	UPROPERTY(EditDefaultsOnly)
	bool bIsActivatedByDefault = false;

	/**
	 * The visibility the widget should have when activated.
	 * This property may not be used if GetDesiredVisibility() is overridden.
	 */
	UPROPERTY(EditDefaultsOnly)
	ESlateVisibility ActivatedVisibility = ESlateVisibility::SelfHitTestInvisible;

	/** 
	 * The visibility the widget should have when deactivated.
	 * This property may not be used if GetDesiredVisibility() is overridden.
	 */
	UPROPERTY(EditDefaultsOnly)
	ESlateVisibility DeactivatedVisibility = ESlateVisibility::Collapsed;

	/**
	 * May this widget ever support player input?
	 * If this is deactivated, all input related functions will be deactivated and throw warnings/fail when used.
	 * Leaving the option deactivated is recommended nevertheless to ease debugging of input related
	 * issues as it reduces the number of potential widgets that could cause such issues.
	 */
	UPROPERTY(EditDefaultsOnly)
	bool bAllowInput = false;

	/**
	 * Should the focus reset function be called whenever the widget is activated?
	 * This setting only has an effect when bAllowInput is set to true.
	 */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bAllowInput"))
	bool bResetFocusOnActivation = true;

	UFUNCTION(BlueprintPure)
	ESlateVisibility GetDesiredVisibility() const;

	UFUNCTION(BlueprintPure)
	bool IsActivated() const;

	UFUNCTION(BlueprintCallable)
	void Activate();

	UFUNCTION(BlueprintCallable)
	void Deactivate();

	/** Enforce a visibility update. Otherwise visibility is only updated on Activation/Deactivation */
	UFUNCTION(BlueprintCallable)
	void ForceUpdateVisibility();

protected:
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Input")
	UUMGInputActionBindingStack* GetInputActionBindingStack();
	
	/** Get the desired visibility based on the current widget state */
	virtual ESlateVisibility NativeGetDesiredVisibility() const;

	/**
	 * Determine desired widget visibility in Blueprint. 
	 * @returns event reply whether the blueprint function returned a value that should be used.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply BlueprintGetDesiredVisibility(ESlateVisibility& OutVisibility) const;

	/**
	 * By default, activation events result in an immediate change of visibility.
	 * You can change this behavior by overriding this function.
	 * This should be done for widgets that shall have transition animations for activation.
	 */
	virtual void NativeUpdateVisibility();

	/**
	 * Blueprint implementation of visibility update.
	 * @returns event reply whether the blueprint function was implemented.
	 */
	UFUNCTION(BlueprintImplementableEvent)
	FEventReply BlueprintUpdateVisibility() const;

	/** Called whenever the widget was activated. */
	UFUNCTION(BlueprintImplementableEvent)
	void ProcessWidgetActivated();

	/** Called whenever the widget was deactivated. */
	UFUNCTION(BlueprintImplementableEvent)
	void ProcessWidgetDeactivated();

	// - UUserWidget
	void NativeOnInitialized() override;
	FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	// - IUserFocusResetableWidget
	bool ResetUserFocus_Implementation() override;
	// --

private:
	/** Is the widget currently activated? */
	UPROPERTY(VisibleAnywhere)
	bool bIsActivated = false;

	/** Input action binding stack. To be lazily created by GetInputActionBindingStack() function. */
	UPROPERTY(Transient)
	UUMGInputActionBindingStack* InputActionBindingStack;
};
