// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "UMGInputBinding.generated.h"

class UUserWidget;

/** Which input events to react to/consume in UMG input action bindings. Compare EInputEvent */
UENUM(BlueprintType)
enum class EUMGInputActionKeyEvent : uint8
{
	// Don't react to/consume any key events
	None,
	// Once on initial key press
	KeyDown,
	// Once as soon as the key was let go
	KeyUp,
	// Every frame during which the key is still down, excluding the first key frame
	KeyHeldContinuous,
	// Once after the initial key down and a specified time has passed
	KeyHeldTimer,
	// React to all key events
	Any
};

/**
 * Utility struct used to bind delegates to an input action.
 * Only works for Input ACTION mappings, not Axis mappings.
 */
USTRUCT(BlueprintType)
struct OPENUNREALUTILITIES_API FUMGInputAction
{
	GENERATED_BODY()
public:
	/** Name of the input action as registered in the InputSettings/PlayerInput. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActionName = NAME_None;
	
	/** Which key event(s) will provoke the callback delegate being broadcast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUMGInputActionKeyEvent ReactEvent = EUMGInputActionKeyEvent::None;

	/** Required time to hold the key for a reaction if ReactEvent is set to KeyHeldTimer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HoldTime = 0.f;

	/** Which key event(s) to consume. Can be a subset or a superset of ReactEvents. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUMGInputActionKeyEvent ConsumeEvent = EUMGInputActionKeyEvent::None;

	/** Bindings with this action will be removed/destroyed as soon as it was broadcast once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsOneshot = false;

	FORCEINLINE bool operator==(const FUMGInputAction& Other) const
	{
		return ActionName == Other.ActionName && ReactEvent == Other.ReactEvent && 
			ConsumeEvent == Other.ConsumeEvent && bIsOneshot == Other.bIsOneshot;
	}
};

DECLARE_DYNAMIC_DELEGATE_OneParam(FUMGInputActionDelegate, EUMGInputActionKeyEvent, SourceEvent);

/**
 * Utility object that allows processing UMG input/focus events and detect and forward events based on
 * Input Action settings from UInputSettings/UPlayerInput system.
 * Contrary to UUserWidget::ListenForInputAction() and it's related functions this works in UI Only input mode
 * and takes the focus path into account to support proper event bubbling along the widget path.
 * This means deeper widgets in the hierarchy get to handle the event first, followed by widgets that are higher
 * up in the hierarchy.
 */
UCLASS(BlueprintType)
class OPENUNREALUTILITIES_API UUMGInputActionBindingStack : public UObject
{
	GENERATED_BODY()
public:
	//-------------
	// Setup
	//------------- 

	/** Set the owning user widget that should be used to setup bindings. */
	UFUNCTION(BlueprintCallable)
	void SetOwningWidget(UUserWidget* InOwningWidget);

	/** Create a UUMGInputActionBindingStack object and initialize it with the specified UUserWidget as outer and owner. */
	UFUNCTION(BlueprintCallable)
	static UUMGInputActionBindingStack* CreateUMGInputActionBindingStack(UUserWidget* InOwningWidget);

	//-------------
	// Action binding/unbinding
	//-------------

	/**
	 * Add an input action binding
	 * @param Action: Action to react to
	 * @param Delegate: Callback delegate that will be called once the action is recognized
	 */
	UFUNCTION(BlueprintCallable)
	void BindAction(FUMGInputAction Action, FUMGInputActionDelegate Delegate);
	
	/** Remove a single input action binding */
	UFUNCTION(BlueprintCallable)
	void RemoveSingleBinding(FUMGInputAction Action, FUMGInputActionDelegate Delegate);
	
	/** Remove all bindings with the specified action*/
	UFUNCTION(BlueprintCallable)
	void RemoveBindings(FUMGInputAction Action);
	
	/** Remove all bindings that have delegates with the specified target object */
	UFUNCTION(BlueprintCallable)
	void RemoveBindingByObject(UObject* TargetObject);
	
	/** Remove all bindings no matter the action or delegate */
	UFUNCTION(BlueprintCallable)
	void RemoveAllBindings();

	//-------------
	// Process Key Events
	//-------------
	// These key events MUST be called in owning user widgets to ensure all key events reach the stack!
	// They can be either bound to the native events or the blueprint events. Both works equally well.
	//-------------

	/** Link this in a widgets KeyDown event and pass the event reply on */
	UFUNCTION(BlueprintCallable)
	FEventReply ProcessOnKeyDown(FGeometry MyGeometry, FKeyEvent InKeyEvent);
	
	/** Link this in a widgets KeyUp event and pass the event reply on */
	UFUNCTION(BlueprintCallable)
	FEventReply ProcessOnKeyUp(FGeometry MyGeometry, FKeyEvent InKeyEvent);
	
private:
	// Can be used to retrieve PlayerInput and InputComponent
	UPROPERTY()
	TSoftObjectPtr<APlayerController> OwningPlayer;

	struct FUMGInputActionBinding
	{
		FUMGInputActionBinding(FUMGInputAction InSignature, FUMGInputActionDelegate InDelegate) :
			BindingSignature(InSignature), Delegate(InDelegate) {}

		FUMGInputAction BindingSignature;

		FUMGInputActionDelegate Delegate;

		// If the delegate was called for a KeyHeldTimer type event.
		// Not used for any of the other event types.
		bool bKeyHeldTimerCalled = false;

		FORCEINLINE bool operator==(const FUMGInputActionBinding& Other) const
		{
			return BindingSignature == Other.BindingSignature && Delegate == Other.Delegate;
		}
	};

	TArray<FUMGInputActionBinding> BindingStack;

	int32 GetFirstBindingWithKey(FKey Key) const;

	FEventReply ProcessBindingMatch(int32 BindingIndex, EUMGInputActionKeyEvent Event);
};
