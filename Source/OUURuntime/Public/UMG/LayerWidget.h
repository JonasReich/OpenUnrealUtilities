// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "UMG/UserFocusResetableWidget.h"
#include "LayerWidget.generated.h"

class UUMGInputActionBindingStack;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnRequestResetFocusToHighestLayer);

/**
 * Widget that acts as a UI layer which are managed by layer stack widgets (see LayerStackWidget.h).
 * Layer widgets have an activation state but have two key differences to activatable widgets:
 * 1. Layer widgets have a limited set of valid visibilites (Collapsed, Visible, HitTestInvisible),
 *		because layers are always assumed to be full-screen and blocking if interactable.
 * 2. Layer widgets manage their activation state internally/across layers and cannot be explicitly
 *		activated or deactivated from external code. They determine their activation state implicitly
 *		from the visibility of contained widgets (see UpdateLayer() and CheckIsInputVisible()).
 */
UCLASS()
class OUURUNTIME_API UOUULayerWidget : public UUserWidget,
	public IUserFocusResetableWidget,
	public TUserFocusResetableWidget_Impl<UOUULayerWidget>
{
	GENERATED_BODY()
public:

	/**
	 * May this widget ever support player input?
	 * If this is deactivated, all input related functions will be deactivated and throw warnings/fail when used.
	 * Leaving the option deactivated is recommended nevertheless to ease debugging of input related
	 * issues as it reduces the number of potential widgets that could cause such issues.
	 */
	UPROPERTY(EditDefaultsOnly)
	bool bAllowInput = false;

	/** May this layer conceal + hide layers below when it's active? */
	UPROPERTY(EditDefaultsOnly)
	bool bConcealLayersBelow = false;

	/** May this layer be concealed/hidden by a concealing layer above in the stack? */
	UPROPERTY(EditDefaultsOnly)
	bool bMayBeConcealedFromAbove = false;

	/**
	 * Called when this layer wants to reset the focus via the layer stack starting from the top-most layer
	 * Must be bound and handled by the layer stack.
	 */
	FOnRequestResetFocusToHighestLayer OnRequestResetFocusFromTopLayer;

	/**
	 * Called by layer stack to tick/update layer properties, visibility, etc.
	 * This function must be called on the topmost layer first and go from there downwards through the stack
	 * to ensure proper layer concealment, focus restoration, etc.
	 */
	virtual void UpdateLayer(const UOUULayerWidget* LayerAbove);

	/**
	 * If this layer is actively concealing layers below
	 */
	bool IsActivelyConcealing() const;

	/**
	 * If this layer was concealed from above.
	 * This value is always required even when bMayBeConcealedFromAbove is false, because the value must be passed
	 * along to layers below so they can be concealed (layers may be skipped).
	 */
	bool IsConcealed() const;

	/** Get the result of the last CheckIsLayerInputVisible check */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Is Input Visible (Cached)"))
	bool IsLayerInputVisible() const;

protected:	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Input")
	UUMGInputActionBindingStack* GetInputActionBindingStack();

	// - UUserWidget
	virtual void NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent) override;
	// - IUserFocusResetableWidget
	virtual bool ResetUserFocus_Implementation() override;
	// --

	ESlateVisibility GetDesiredVisibility() const;

	void CheckLayerVisibility();

private:
	/** Input action binding stack. To be lazily created by GetInputActionBindingStack() function. */
	UPROPERTY(Transient)
	UUMGInputActionBindingStack* InputActionBindingStack;
	
	/**
	 * Is there a concealing layer above? This info is still required even if the layer itself cannot be concealed,
	 * because the value is passed down the widget stack to layers below.
	 */
	UPROPERTY(VisibleAnywhere)
	bool bHasConcealingLayerAbove = false;

	/**
	 * Cached result of last CheckIsLayerInputVisible() call, if any of the child widgets are visible.
	 * This value may be out of date, so be careful when using it!
	 */
	UPROPERTY(VisibleAnywhere)
	bool bIsLayerVisible = false;

	/**
	 * Cached result of last CheckIsLayerInputVisible() call, if any of the child widgets are input visible.
	 * This value may be out of date, so be careful when using it!
	 */
	UPROPERTY(VisibleAnywhere)
	bool bIsLayerInputVisible = false;

	FWeakWidgetPath LastValidFocusPath;

	bool DoesPathContainGameViewport(const FWidgetPath& NewWidgetPath);
};
