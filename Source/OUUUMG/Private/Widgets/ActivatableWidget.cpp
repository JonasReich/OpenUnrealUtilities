// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Widgets/ActivatableWidget.h"

#include "LogOpenUnrealUtilities.h"
#include "UMG/UMGUtils.h"
#include "UMGInputBinding.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

UUMGInputActionBindingStack_DEPRECATED* UOUUActivatableWidget_DEPRECATED::GetInputActionBindingStack()
{
	if (!bAllowInput)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("GetInputActionBindingStack was called on Activatable Widget '%s' "
				 "that is not marked as bAllowInput!"));
		return nullptr;
	}
	if (!IsValid(InputActionBindingStack))
	{
		InputActionBindingStack = UUMGInputActionBindingStack_DEPRECATED::CreateUMGInputActionBindingStack(this);
	}
	return InputActionBindingStack;
}

ESlateVisibility UOUUActivatableWidget_DEPRECATED::NativeGetDesiredVisibility() const
{
	return bIsActivated ? ActivatedVisibility : DeactivatedVisibility;
}

void UOUUActivatableWidget_DEPRECATED::NativeUpdateVisibility()
{
	SetVisibility(GetDesiredVisibility());
}

void UOUUActivatableWidget_DEPRECATED::NativeOnInitialized()
{
	bIsActivated = bIsActivatedByDefault;
	ForceUpdateVisibility();
	Super::NativeOnInitialized();
}

FReply UOUUActivatableWidget_DEPRECATED::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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

FReply UOUUActivatableWidget_DEPRECATED::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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

bool UOUUActivatableWidget_DEPRECATED::ResetUserFocus_Implementation()
{
	return ResetUserFocus_TemplateImplementation();
}

ESlateVisibility UOUUActivatableWidget_DEPRECATED::GetDesiredVisibility() const
{
	ESlateVisibility OutVisibility;
	FEventReply EventReply = BlueprintGetDesiredVisibility(OutVisibility);
	if (EventReply.NativeReply.IsEventHandled())
		return OutVisibility;

	return NativeGetDesiredVisibility();
}

bool UOUUActivatableWidget_DEPRECATED::IsActivated() const
{
	return bIsActivated;
}

void UOUUActivatableWidget_DEPRECATED::Activate()
{
	if (bIsActivated)
		return;

	bIsActivated = true;
	ForceUpdateVisibility();
}

void UOUUActivatableWidget_DEPRECATED::Deactivate()
{
	if (!bIsActivated)
		return;

	bIsActivated = false;
	ForceUpdateVisibility();
}

void UOUUActivatableWidget_DEPRECATED::ForceUpdateVisibility()
{
	FEventReply Reply = BlueprintUpdateVisibility();
	if (!Reply.NativeReply.IsEventHandled())
	{
		NativeUpdateVisibility();
	}
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS
