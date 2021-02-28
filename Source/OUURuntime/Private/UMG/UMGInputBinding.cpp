// Copyright (c) 2021 Jonas Reich

#include "UMG/UMGInputBinding.h"
#include "Blueprint/UserWidget.h"
#include "LogOpenUnrealUtilities.h"
#include "GameFramework/PlayerInput.h"

void UUMGInputActionBindingStack::SetOwningPlayerInput(UPlayerInput* InOwningPlayerInput)
{
	OwningPlayerInput = InOwningPlayerInput;
}

UPlayerInput* UUMGInputActionBindingStack::GetOwningPlayerInput() const
{
	return OwningPlayerInput.IsValid() ? OwningPlayerInput.Get() : nullptr;
}

UUMGInputActionBindingStack* UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(UUserWidget* InOwningWidget)
{
	if (!IsValid(InOwningWidget))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Could not create UMGInputActionBindingStack with invalid OwningWidget!"));
		return nullptr;
	}

	APlayerController* OwningPlayer = InOwningWidget->GetOwningPlayer();
	if (!ensure(IsValid(OwningPlayer)))
		return nullptr;

	UUMGInputActionBindingStack* NewStack = NewObject<UUMGInputActionBindingStack>(InOwningWidget);
	NewStack->SetOwningPlayerInput(OwningPlayer->PlayerInput);
	return NewStack;
}

void UUMGInputActionBindingStack::BindAction(FUMGInputAction Action, FUMGInputActionDelegate Delegate)
{
	BindingStack.Add({ Action, Delegate });
}

void UUMGInputActionBindingStack::RemoveSingleBinding(FUMGInputAction Action, FUMGInputActionDelegate Delegate)
{
	BindingStack.Remove({ Action, Delegate });
}

void UUMGInputActionBindingStack::RemoveBindings(FUMGInputAction Action)
{
	BindingStack.RemoveAll([&Action](const FUMGInputActionBinding& Binding) -> bool {
		return Binding.BindingSignature == Action;
	});
}

void UUMGInputActionBindingStack::RemoveBindingsByObject(UObject* TargetObject)
{
	BindingStack.RemoveAll([&TargetObject](const FUMGInputActionBinding& Binding) -> bool {
		return Binding.Delegate.IsBoundToObject(TargetObject);
	});
}

void UUMGInputActionBindingStack::RemoveAllBindings()
{
	BindingStack.Empty();
}

int32 UUMGInputActionBindingStack::GetNumBindingsToObject(UObject* Object)
{
	int32 Count = 0;
	for (const FUMGInputActionBinding& Binding : BindingStack)
	{
		if (Binding.Delegate.IsBoundToObject(Object))
			Count++;
	}
	return Count;
}

FEventReply UUMGInputActionBindingStack::ProcessOnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	bool bHandled = ProcessKeyEvent(MyGeometry, InKeyEvent);
	CleanUpStack();
	return bHandled;
}

FEventReply UUMGInputActionBindingStack::ProcessOnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	bool bHandled = ProcessKeyEvent(MyGeometry, InKeyEvent);
	CleanUpStack();
	return bHandled;
}

int32 UUMGInputActionBindingStack::GetFirstBindingWithKey(FKey Key) const
{
	for (int32 i = 0; i < BindingStack.Num(); i++)
	{
		const FUMGInputActionBinding& Binding = BindingStack[i];
		const TArray<FInputActionKeyMapping>& Actions = OwningPlayerInput->GetKeysForAction(Binding.BindingSignature.ActionName);
		bool bMatchesKey = Actions.ContainsByPredicate([&Key](const FInputActionKeyMapping& Action) {
			return Action.Key == Key;
		});
		if (bMatchesKey) return i;
	}
	return INDEX_NONE;
}

bool UUMGInputActionBindingStack::ProcessKeyEvent(FGeometry MyGeometry, FKeyEvent InKeyEvent)
{
	FKey Key = InKeyEvent.GetKey();

	if (!ensure(OwningPlayerInput.IsValid()))
		return false;

	for (int32 Idx = 0; Idx < BindingStack.Num(); Idx++)
	{
		FUMGInputActionBinding& Binding = BindingStack[Idx];
		const TArray<FInputActionKeyMapping>& Actions = OwningPlayerInput->GetKeysForAction(Binding.BindingSignature.ActionName);
		bool bMatchesKey = Actions.ContainsByPredicate([&Key](const FInputActionKeyMapping& Action) {
			return Action.Key == Key;
		});

		if(!bMatchesKey)
			continue;

		switch (Binding.BindingSignature.ReactEvent)
		{
		case EUMGInputActionKeyEvent::KeyDown:
		{
			if (OwningPlayerInput->WasJustPressed(Key))
			{
				if (ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyDown))
					return true;
			}
			break;
		}
		case EUMGInputActionKeyEvent::KeyUp:
		{
			if (OwningPlayerInput->WasJustReleased(Key))
			{
				if (ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyUp))
					return true;
			}
			break;
		}
		case EUMGInputActionKeyEvent::KeyHeldContinuous:
		{
			if (OwningPlayerInput->WasJustPressed(Key) == false && OwningPlayerInput->IsPressed(Key))
			{
				if (ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyHeldContinuous))
					return true;
			}
			break;
		}
		case EUMGInputActionKeyEvent::KeyHeldTimer:
		{
			float TimeDown = OwningPlayerInput->GetTimeDown(Key);
			if (TimeDown < Binding.BindingSignature.HoldTime)
			{
				Binding.bKeyHeldTimerCalled = false;
			}
			if (TimeDown >= Binding.BindingSignature.HoldTime && Binding.bKeyHeldTimerCalled == false)
			{
				Binding.bKeyHeldTimerCalled = true;
				if (ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyHeldTimer))
					return true;
			}
			break;
		}
		case EUMGInputActionKeyEvent::Any:
		{
			if (ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::Any))
				return true;
		}
		default:
			break;
		}
	}

	return false;
}

bool UUMGInputActionBindingStack::ProcessBindingMatch(int32 BindingIndex, EUMGInputActionKeyEvent Event)
{
	check(BindingStack.IsValidIndex(BindingIndex));
	FUMGInputActionBinding& Binding = BindingStack[BindingIndex];
	Binding.Delegate.ExecuteIfBound(Event);
	EUMGInputActionKeyEventConsumeMode ConsumeMode = Binding.BindingSignature.ConsumeMode;
	if (Binding.BindingSignature.bIsOneshot)
	{
		IndicesToRemoveThisFrame.Add(BindingIndex);
	}
	return ConsumeMode == EUMGInputActionKeyEventConsumeMode::Same || ConsumeMode == EUMGInputActionKeyEventConsumeMode::All;
}

void UUMGInputActionBindingStack::CleanUpStack()
{
	int32 NumRemoved = 0;
	for (int32 Index : IndicesToRemoveThisFrame)
	{
		BindingStack.RemoveAt(Index - NumRemoved);
		NumRemoved++;
	}
	IndicesToRemoveThisFrame.Empty();
}
