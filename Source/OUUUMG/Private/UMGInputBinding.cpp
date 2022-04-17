// Copyright (c) 2022 Jonas Reich

#include "UMGInputBinding.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerInput.h"
#include "LogOpenUnrealUtilities.h"

void UUMGInputActionBindingStack_DEPRECATED::SetOwningPlayerInput(UPlayerInput* InOwningPlayerInput)
{
	OwningPlayerInput = InOwningPlayerInput;
}

UPlayerInput* UUMGInputActionBindingStack_DEPRECATED::GetOwningPlayerInput() const
{
	return OwningPlayerInput.IsValid() ? OwningPlayerInput.Get() : nullptr;
}

UUMGInputActionBindingStack_DEPRECATED* UUMGInputActionBindingStack_DEPRECATED::CreateUMGInputActionBindingStack(
	UUserWidget* InOwningWidget)
{
	if (!IsValid(InOwningWidget))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Error,
			TEXT("Could not create UMGInputActionBindingStack with invalid OwningWidget!"));
		return nullptr;
	}

	APlayerController* OwningPlayer = InOwningWidget->GetOwningPlayer();
	if (!ensure(IsValid(OwningPlayer)))
		return nullptr;

	UUMGInputActionBindingStack_DEPRECATED* NewStack =
		NewObject<UUMGInputActionBindingStack_DEPRECATED>(InOwningWidget);
	NewStack->SetOwningPlayerInput(OwningPlayer->PlayerInput);
	return NewStack;
}

void UUMGInputActionBindingStack_DEPRECATED::BindAction(FUMGInputAction Action, FUMGInputActionDelegate Delegate)
{
	BindingStack.Add({Action, Delegate});
}

void UUMGInputActionBindingStack_DEPRECATED::RemoveSingleBinding(
	FUMGInputAction Action,
	FUMGInputActionDelegate Delegate)
{
	BindingStack.Remove({Action, Delegate});
}

void UUMGInputActionBindingStack_DEPRECATED::RemoveBindings(FUMGInputAction Action)
{
	BindingStack.RemoveAll(
		[&Action](const FUMGInputActionBinding& Binding) -> bool { return Binding.BindingSignature == Action; });
}

void UUMGInputActionBindingStack_DEPRECATED::RemoveBindingsByObject(UObject* TargetObject)
{
	BindingStack.RemoveAll([&TargetObject](const FUMGInputActionBinding& Binding) -> bool {
		return Binding.Delegate.IsBoundToObject(TargetObject);
	});
}

void UUMGInputActionBindingStack_DEPRECATED::RemoveAllBindings()
{
	BindingStack.Empty();
}

int32 UUMGInputActionBindingStack_DEPRECATED::GetNumBindingsToObject(UObject* Object)
{
	int32 Count = 0;
	for (const FUMGInputActionBinding& Binding : BindingStack)
	{
		if (Binding.Delegate.IsBoundToObject(Object))
			Count++;
	}
	return Count;
}

FEventReply UUMGInputActionBindingStack_DEPRECATED::ProcessOnKeyDown(
	const FGeometry& MyGeometry,
	const FKeyEvent& InKeyEvent)
{
	bool bHandled = ProcessKeyEvent(MyGeometry, InKeyEvent);
	CleanUpStack();
	return bHandled;
}

FEventReply UUMGInputActionBindingStack_DEPRECATED::ProcessOnKeyUp(
	const FGeometry& MyGeometry,
	const FKeyEvent& InKeyEvent)
{
	bool bHandled = ProcessKeyEvent(MyGeometry, InKeyEvent);
	CleanUpStack();
	return bHandled;
}

int32 UUMGInputActionBindingStack_DEPRECATED::GetFirstBindingWithKey(FKey Key) const
{
	for (int32 i = 0; i < BindingStack.Num(); i++)
	{
		const FUMGInputActionBinding& Binding = BindingStack[i];
		const TArray<FInputActionKeyMapping>& Actions =
			OwningPlayerInput->GetKeysForAction(Binding.BindingSignature.ActionName);
		bool bMatchesKey =
			Actions.ContainsByPredicate([&Key](const FInputActionKeyMapping& Action) { return Action.Key == Key; });
		if (bMatchesKey)
			return i;
	}
	return INDEX_NONE;
}

bool UUMGInputActionBindingStack_DEPRECATED::ProcessKeyEvent(FGeometry MyGeometry, FKeyEvent InKeyEvent)
{
	FKey Key = InKeyEvent.GetKey();

	if (!ensure(OwningPlayerInput.IsValid()))
		return false;

	for (int32 Idx = 0; Idx < BindingStack.Num(); Idx++)
	{
		FUMGInputActionBinding& Binding = BindingStack[Idx];
		const TArray<FInputActionKeyMapping>& Actions =
			OwningPlayerInput->GetKeysForAction(Binding.BindingSignature.ActionName);
		bool bMatchesKey =
			Actions.ContainsByPredicate([&Key](const FInputActionKeyMapping& Action) { return Action.Key == Key; });

		if (!bMatchesKey)
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
		default: break;
		}
	}

	return false;
}

bool UUMGInputActionBindingStack_DEPRECATED::ProcessBindingMatch(int32 BindingIndex, EUMGInputActionKeyEvent Event)
{
	check(BindingStack.IsValidIndex(BindingIndex));
	FUMGInputActionBinding& Binding = BindingStack[BindingIndex];
	Binding.Delegate.ExecuteIfBound(Event);
	EUMGInputActionKeyEventConsumeMode ConsumeMode = Binding.BindingSignature.ConsumeMode;
	if (Binding.BindingSignature.bIsOneshot)
	{
		IndicesToRemoveThisFrame.Add(BindingIndex);
	}
	return ConsumeMode == EUMGInputActionKeyEventConsumeMode::Same
		|| ConsumeMode == EUMGInputActionKeyEventConsumeMode::All;
}

void UUMGInputActionBindingStack_DEPRECATED::CleanUpStack()
{
	int32 NumRemoved = 0;
	for (int32 Index : IndicesToRemoveThisFrame)
	{
		BindingStack.RemoveAt(Index - NumRemoved);
		NumRemoved++;
	}
	IndicesToRemoveThisFrame.Empty();
}
