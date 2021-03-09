// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Widgets/ActivatableWidget.h"
#include "LogOpenUnrealUtilities.h"
#include "UMGInputBinding.h"
#include "UMG/UMGUtils.h"

UUMGInputActionBindingStack* UOUUActivatableWidget::GetInputActionBindingStack()
{
	if (!bAllowInput)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("GetInputActionBindingStack was called on Activatable Widget '%s' that is not marked as bAllowInput!"));
		return nullptr;
	}
	if (!IsValid(InputActionBindingStack))
	{
		InputActionBindingStack = UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(this);
	}
	return InputActionBindingStack;
}

ESlateVisibility UOUUActivatableWidget::NativeGetDesiredVisibility() const
{
	return bIsActivated ? ActivatedVisibility : DeactivatedVisibility;
}

void UOUUActivatableWidget::NativeUpdateVisibility()
{
	SetVisibility(GetDesiredVisibility());
}

void UOUUActivatableWidget::NativeOnInitialized()
{
	bIsActivated = bIsActivatedByDefault;
	ForceUpdateVisibility();
	Super::NativeOnInitialized();
}

FReply UOUUActivatableWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsValid(InputActionBindingStack))
	{
		auto Result = InputActionBindingStack->ProcessOnKeyDown(InGeometry, InKeyEvent);
		if (Result.NativeReply.IsEventHandled())
			return Result.NativeReply;
	}
	// If the event was not handled by the stack,
	// the blueprint should still get the opportunity to do so
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UOUUActivatableWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (IsValid(InputActionBindingStack))
	{
		auto Result = InputActionBindingStack->ProcessOnKeyUp(InGeometry, InKeyEvent);
		if (Result.NativeReply.IsEventHandled())
			return Result.NativeReply;
	}
	// If the event was not handled by the stack,
	// the blueprint should still get the opportunity to do so
	return Super::NativeOnKeyUp(InGeometry, InKeyEvent);
}

bool UOUUActivatableWidget::ResetUserFocus_Implementation()
{
	return ResetUserFocus_TemplateImplementation();
}

ESlateVisibility UOUUActivatableWidget::GetDesiredVisibility() const
{
	ESlateVisibility OutVisibility;
	FEventReply EventReply = BlueprintGetDesiredVisibility(OutVisibility);
	if (EventReply.NativeReply.IsEventHandled())
		return OutVisibility;
	
	return NativeGetDesiredVisibility();
}

bool UOUUActivatableWidget::IsActivated() const
{
	return bIsActivated;
}

void UOUUActivatableWidget::Activate()
{
	if (bIsActivated)
		return;

	bIsActivated = true;
	ForceUpdateVisibility();
}

void UOUUActivatableWidget::Deactivate()
{
	if (!bIsActivated)
		return;

	bIsActivated = false;
	ForceUpdateVisibility();
}

void UOUUActivatableWidget::ForceUpdateVisibility()
{
	FEventReply Reply = BlueprintUpdateVisibility();
	if (!Reply.NativeReply.IsEventHandled())
	{
		NativeUpdateVisibility();
	}
}

