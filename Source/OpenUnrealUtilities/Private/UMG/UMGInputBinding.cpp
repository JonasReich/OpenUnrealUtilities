// Copyright (c) 2020 Jonas Reich

#include "UMG/UMGInputBinding.h"
#include "Blueprint/UserWidget.h"

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

}

void UUMGInputActionBindingStack::RemoveSingleBinding(FUMGInputAction Action, FUMGInputActionDelegate Delegate)
{

}

void UUMGInputActionBindingStack::RemoveBindings(FUMGInputAction Action)
{

}

void UUMGInputActionBindingStack::RemoveBindingByObject(UObject* TargetObject)
{

}

void UUMGInputActionBindingStack::RemoveAllBindings()
{

}

FEventReply UUMGInputActionBindingStack::ProcessOnKeyDown(FGeometry MyGeometry, FKeyEvent InKeyEvent)
{
	return FEventReply(false);
}

FEventReply UUMGInputActionBindingStack::ProcessOnKeyUp(FGeometry MyGeometry, FKeyEvent InKeyEvent)
{
	return FEventReply(false);
}

void UUMGInputActionBindingStack::ProcessOnRemovedFromFocusPath(const FFocusEvent& InFocusEvent)
{

}
