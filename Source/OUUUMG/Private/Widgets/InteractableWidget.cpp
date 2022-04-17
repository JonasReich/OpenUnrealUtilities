// Copyright (c) 2022 Jonas Reich

#include "Widgets/InteractableWidget.h"

#include "UMGInputBinding.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

UUMGInputActionBindingStack_DEPRECATED* UOUUInteractableWidget_DEPRECATED::GetInputActionBindingStack()
{
	if (!IsValid(InputActionBindingStack))
	{
		InputActionBindingStack = UUMGInputActionBindingStack_DEPRECATED::CreateUMGInputActionBindingStack(this);
	}
	return InputActionBindingStack;
}

FReply UOUUInteractableWidget_DEPRECATED::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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

FReply UOUUInteractableWidget_DEPRECATED::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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

bool UOUUInteractableWidget_DEPRECATED::ResetUserFocus_Implementation()
{
	return ResetUserFocus_TemplateImplementation();
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS
