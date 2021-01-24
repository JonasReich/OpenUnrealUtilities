// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Components/SlateWrapperTypes.h"
#include "UMGInputBinding.generated.h"

class UUserWidget;

/** Which input events to react to in UMG input action bindings. Compare EInputEvent */
UENUM(BlueprintType)
enum class EUMGInputActionKeyEvent : uint8
{
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
 * Which input events to handle/consume in UMG input action bindings.
 * Values are defined in relation to EUMGInputActionKeyEvents that trigger a response.
 */
UENUM(BlueprintType)
enum class EUMGInputActionKeyEventConsumeMode : uint8
{
	// Don't consume any key events
	None,
	// Consume all key events that trigger a reaction
	Same,
	// Consume all key events
	All
};


/**
 * Utility struct used to bind delegates to an input action.
 * Only works for Input ACTION mappings, not Axis mappings.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FUMGInputAction
{
	GENERATED_BODY()
public:
	FUMGInputAction() = default;
	FUMGInputAction(FName InActionName, EUMGInputActionKeyEvent InReactEvent, EUMGInputActionKeyEventConsumeMode InConsumeMode = EUMGInputActionKeyEventConsumeMode::Same,
		bool bInIsOneshot = false, float InHoldTime = 0.f) :

		ActionName(InActionName),
		ReactEvent(InReactEvent),
		ConsumeMode(InConsumeMode),
		bIsOneshot(bInIsOneshot),
		HoldTime(InHoldTime)
	{
	}

	/** Name of the input action as registered in the InputSettings/PlayerInput. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ActionName = NAME_None;
	
	/** Which key event(s) will provoke the callback delegate being broadcast. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUMGInputActionKeyEvent ReactEvent = EUMGInputActionKeyEvent::KeyDown;

	/** Which key event(s) to consume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EUMGInputActionKeyEventConsumeMode ConsumeMode = EUMGInputActionKeyEventConsumeMode::Same;

	/** Bindings with this action will be removed/destroyed as soon as it was broadcast once. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bIsOneshot = false;

	/** Required time to hold the key for a reaction if ReactEvent is set to KeyHeldTimer */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HoldTime = 0.f;

	FORCEINLINE bool operator==(const FUMGInputAction& Other) const
	{
		return ActionName == Other.ActionName && ReactEvent == Other.ReactEvent && 
			ConsumeMode == Other.ConsumeMode && bIsOneshot == Other.bIsOneshot;
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
class OUURUNTIME_API UUMGInputActionBindingStack : public UObject
{
	GENERATED_BODY()
public:
	//-------------
	// Setup
	//------------- 

	UFUNCTION(BlueprintCallable)
	void SetOwningPlayerInput(UPlayerInput* InOwningPlayerInput);
	
	UFUNCTION(BlueprintPure)
	UPlayerInput* GetOwningPlayerInput() const;

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
	void RemoveBindingsByObject(UObject* TargetObject);
	
	/** Remove all bindings no matter the action or delegate */
	UFUNCTION(BlueprintCallable)
	void RemoveAllBindings();

	/** @returns the number of bindings in the stack bound to the specified object */
	UFUNCTION(BlueprintPure)
	int32 GetNumBindingsToObject(UObject* Object);

	//-------------
	// Process Key Events
	//-------------
	// These key events MUST be called in owning user widgets to ensure all key events reach the stack!
	// They can be either bound to the native events or the blueprint events. Both works equally well.
	//-------------

	/** Link this in a widgets KeyDown event and pass the event reply on */
	UFUNCTION(BlueprintCallable)
	FEventReply ProcessOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	
	/** Link this in a widgets KeyUp event and pass the event reply on */
	UFUNCTION(BlueprintCallable)
	FEventReply ProcessOnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent);
	
private:
	UPROPERTY()
	TSoftObjectPtr<UPlayerInput> OwningPlayerInput;

	TArray<int32> IndicesToRemoveThisFrame;

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

	bool ProcessKeyEvent(FGeometry MyGeometry, FKeyEvent InKeyEvent);

	bool ProcessBindingMatch(int32 BindingIndex, EUMGInputActionKeyEvent Event);

	void CleanUpStack();
};
