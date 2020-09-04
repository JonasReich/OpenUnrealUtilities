// Copyright (c) 2020 Jonas Reich

#include "UMG/UMGInputBinding.h"
#include "Blueprint/UserWidget.h"
#include "OpenUnrealUtilities.h"
#include <GameFramework/PlayerInput.h>

void UUMGInputActionBindingStack::SetOwningWidget(UUserWidget* InOwningWidget)
{
	OwningPlayer = InOwningWidget->GetOwningPlayer();
	ensure(IsValid(OwningPlayer.Get()));
}

UUMGInputActionBindingStack* UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(UUserWidget* InOwningWidget)
{
	UUMGInputActionBindingStack* NewStack = NewObject<UUMGInputActionBindingStack>(InOwningWidget);
	NewStack->SetOwningWidget(InOwningWidget);
	return NewStack;
}

void UUMGInputActionBindingStack::BindAction(FUMGInputAction Action, FUMGInputActionDelegate Delegate)
{
	if (Action.ReactEvent == EUMGInputActionKeyEvent::None)
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Action '%s' could not be bound in action stack because ReactEvent is None!"));
		return;
	}

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

void UUMGInputActionBindingStack::RemoveBindingByObject(UObject* TargetObject)
{
	BindingStack.RemoveAll([&TargetObject](const FUMGInputActionBinding& Binding) -> bool {
		return Binding.Delegate.IsBoundToObject(TargetObject);
	});
}

void UUMGInputActionBindingStack::RemoveAllBindings()
{
	BindingStack.Empty();
}

FEventReply UUMGInputActionBindingStack::ProcessOnKeyDown(FGeometry MyGeometry, FKeyEvent InKeyEvent)
{
	FKey Key = InKeyEvent.GetKey();
	int32 Idx = GetFirstBindingWithKey(Key);
	if (Idx == INDEX_NONE)
		return FEventReply(false);

	UPlayerInput* Input = OwningPlayer->PlayerInput;

	FUMGInputActionBinding& Binding = BindingStack[Idx];
	switch (Binding.BindingSignature.ReactEvent)
	{
	case EUMGInputActionKeyEvent::None:
	{
		ensureMsgf(false, TEXT("ActionEvents should not be registered with ReactEvent == None"));
		return FEventReply(false);
	}
	case EUMGInputActionKeyEvent::KeyDown:
	{
		if (Input->WasJustPressed(Key))
		{
			return ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyDown);
		}
		break;
	}
	case EUMGInputActionKeyEvent::KeyUp:
	{
		if (Input->WasJustReleased(Key))
		{
			return ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyUp);
		}
		break;
	}
	case EUMGInputActionKeyEvent::KeyHeldContinuous:
	{
		if (Input->WasJustPressed(Key) == false && Input->IsPressed(Key))
		{
			return ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyHeldContinuous);
		}
		break;
	}
	case EUMGInputActionKeyEvent::KeyHeldTimer:
	{
		float TimeDown = Input->GetTimeDown(Key);
		if (TimeDown < Binding.BindingSignature.HoldTime)
		{
			Binding.bKeyHeldTimerCalled = false;
		}
		if (TimeDown >= Binding.BindingSignature.HoldTime && Binding.bKeyHeldTimerCalled == false)
		{
			Binding.bKeyHeldTimerCalled = true;
			return ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::KeyHeldTimer);
		}
		break;
	}
	case EUMGInputActionKeyEvent::Any:
	{
		return ProcessBindingMatch(Idx, EUMGInputActionKeyEvent::Any);
	}
	default:
		break;
	}
	
	return FEventReply(false);
}

FEventReply UUMGInputActionBindingStack::ProcessOnKeyUp(FGeometry MyGeometry, FKeyEvent InKeyEvent)
{
	return FEventReply(false);
}

int32 UUMGInputActionBindingStack::GetFirstBindingWithKey(FKey Key) const
{
	for (int32 i = 0; i < BindingStack.Num(); i++)
	{
		const FUMGInputActionBinding& Binding = BindingStack[i];
		const TArray<FInputActionKeyMapping>& Actions = OwningPlayer->PlayerInput->GetKeysForAction(Binding.BindingSignature.ActionName);
		bool bMatchesKey = Actions.ContainsByPredicate([&Key](const FInputActionKeyMapping& Action) {
			return Action.Key == Key;
		});
		if (bMatchesKey) return i;
	}
	return INDEX_NONE;
}

FEventReply UUMGInputActionBindingStack::ProcessBindingMatch(int32 BindingIndex, EUMGInputActionKeyEvent Event)
{
	check(BindingStack.IsValidIndex(BindingIndex));
	FUMGInputActionBinding& Binding = BindingStack[BindingIndex];
	Binding.Delegate.ExecuteIfBound(Event);
	EUMGInputActionKeyEvent ConsumeEvent = Binding.BindingSignature.ConsumeEvent;
	bool bConsume = ConsumeEvent == Event || ConsumeEvent == EUMGInputActionKeyEvent::Any;
	if (Binding.BindingSignature.bIsOneshot)
	{
		BindingStack.RemoveAt(BindingIndex);
	}
	return FEventReply(bConsume);
}
