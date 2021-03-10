// Copyright (c) 2021 Jonas Reich

#include "Widgets/InteractableWidget.h"
#include "UMGInputBinding.h"

UUMGInputActionBindingStack* UOUUInteractableWidget::GetInputActionBindingStack()
{
	if (!IsValid(InputActionBindingStack))
	{
		InputActionBindingStack = UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(this);
	}
	return InputActionBindingStack;
}

FReply UOUUInteractableWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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

FReply UOUUInteractableWidget::NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
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

bool UOUUInteractableWidget::ResetUserFocus_Implementation()
{
	return ResetUserFocus_TemplateImplementation();
}
