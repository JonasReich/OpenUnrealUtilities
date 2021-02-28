// Copyright (c) 2021 Jonas Reich

#include "UMG/LayerStackWidget.h"
#include "Templates/CastObjectRange.h"
#include "UMG/LayerWidget.h"
#include "Components/Overlay.h"
#include "LogOpenUnrealUtilities.h"

UOUULayerStackWidget::UOUULayerStackWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UOUULayerStackWidget::StartNewLinkedStack()
{
	bAddedCorrectly = true;
	AddRecursivelyToPlayerScreenFromRoot();
}

void UOUULayerStackWidget::InsertToLinkedStackAbove(UOUULayerStackWidget* OtherStack)
{
	if (!IsValid(OtherStack))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Cannot insert stack %s above an invalid stack"), *GetName());
		return;
	}

	InsertBetween(OtherStack->GetStackAbove(), OtherStack);
}

void UOUULayerStackWidget::InsertToLinkedStackBelow(UOUULayerStackWidget* OtherStack)
{
	if (!IsValid(OtherStack))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Cannot insert stack %s below an invalid stack"), *GetName());
		return;
	}

	InsertBetween(OtherStack, OtherStack->GetStackBelow());
}

void UOUULayerStackWidget::RemoveFromLinkedStack()
{
	if (UOUULayerStackWidget* StackAbove = GetStackAbove())
	{
		StackAbove->LinkedStackBelow = LinkedStackBelow;
	}

	if (UOUULayerStackWidget* StackBelow = GetStackBelow())
	{
		StackBelow->LinkedStackAbove = LinkedStackAbove;
	}

	LinkedStackAbove.Reset();
	LinkedStackBelow.Reset();
}

bool UOUULayerStackWidget::IsLinkedStackHead() const
{
	if (!bAddedCorrectly)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("LayerStackWidget was not initialized correctly and is not part of a linked stack! "
			"You must call StartNewLinkedStack() to create a new linked stack or one of the insert functions to add a LayerStackWidget to "
			"an existing linked stack before calling any of the other member functions!"));
	}
	
	return LinkedStackAbove.IsValid() == false;
}

bool UOUULayerStackWidget::IsLinkedStackRoot() const
{
	if (!bAddedCorrectly)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("LayerStackWidget was not initialized correctly and is not part of a linked stack! "
			"You must call StartNewLinkedStack() to create a new linked stack or one of the insert functions to add a LayerStackWidget to "
			"an existing linked stack before calling any of the other member functions!"));
	}

	return LinkedStackBelow.IsValid() == false;
}

UOUULayerStackWidget* UOUULayerStackWidget::GetStackAbove() const
{
	if (LinkedStackAbove.IsValid())
		return LinkedStackAbove.Get();
	return nullptr;
}

UOUULayerStackWidget* UOUULayerStackWidget::GetStackBelow() const
{
	if (LinkedStackBelow.IsValid())
		return LinkedStackBelow.Get();
	return nullptr;
}

UOUULayerStackWidget* UOUULayerStackWidget::GetLinkedStackHead()
{
	return IsLinkedStackHead() ? this : LinkedStackAbove->GetLinkedStackHead();
}

UOUULayerStackWidget* UOUULayerStackWidget::GetLinkedStackRoot()
{
	return IsLinkedStackRoot() ? this : LinkedStackBelow->GetLinkedStackRoot();
}

void UOUULayerStackWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!IsValid(WidgetStack))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("WidgetStack overlay widget is not bound in NativeOnInitialized. "
			"This will prevents all of the stack functionality from functioning correctly!"));
		return;
	}

	int32 NumLayersFound = 0;
	for (UOUULayerWidget* Layer : CastObjectRange<UOUULayerWidget>(WidgetStack->GetAllChildren()))
	{
		NumLayersFound++;
		Layer->OnRequestResetFocusFromTopLayer.AddDynamic(this, &UOUULayerStackWidget::HandleRequestResetFocusFromTopLayer);
	}

	UE_CLOG(NumLayersFound == 0, LogOpenUnrealUtilities, Warning, TEXT("WidgetStack overlay widget does not contain any UOUULayerWidget children"));
}

void UOUULayerStackWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (IsLinkedStackHead())
	{
		UpdateLayers(nullptr);
	}
}

void UOUULayerStackWidget::NativeDestruct()
{
	if (IsValid(WidgetStack))
	{
		for (UOUULayerWidget* Layer : CastObjectRange<UOUULayerWidget>(WidgetStack->GetAllChildren()))
		{
			Layer->OnRequestResetFocusFromTopLayer.RemoveDynamic(this, &UOUULayerStackWidget::HandleRequestResetFocusFromTopLayer);
		}
	}
	Super::NativeDestruct();
}

void UOUULayerStackWidget::InsertBetween(UOUULayerStackWidget* NewAbove, UOUULayerStackWidget* NewBelow)
{
	bAddedCorrectly = true;

	LinkedStackAbove = NewAbove;
	LinkedStackBelow = NewBelow;

	if (NewAbove)
	{
		NewAbove->LinkedStackBelow = this;
	}
	if (NewBelow)
	{
		NewBelow->LinkedStackAbove = this;
	}

	AddRecursivelyToPlayerScreenFromRoot();
}

void UOUULayerStackWidget::UpdateLayers(UOUULayerWidget* BottomLayerFromStackAbove)
{
	UOUULayerWidget* LastLayer = BottomLayerFromStackAbove;
	if (IsValid(WidgetStack))
	{
		for (UOUULayerWidget* Layer : CastObjectRange<UOUULayerWidget>(WidgetStack->GetAllChildren()))
		{
			Layer->UpdateLayer(LastLayer);
			LastLayer = Layer;
		}
	}
	if (LinkedStackBelow.IsValid())
	{
		LinkedStackBelow->UpdateLayers(LastLayer);
	}
}

void UOUULayerStackWidget::HandleRequestResetFocusFromTopLayer()
{
	// forward to linked stack head.
	// we do not bind this delegate to linked stack head directly to make it a bit easier to link in new stack widgets after the fact
	GetLinkedStackHead()->ResetUserFocusRecursively();
}

void UOUULayerStackWidget::ResetUserFocusRecursively()
{
	if (IsValid(WidgetStack))
	{
		for (UOUULayerWidget* Layer : CastObjectRange<UOUULayerWidget>(WidgetStack->GetAllChildren()))
		{
			if (IUserFocusResetableWidget::TryResetUserFocusTo(Layer))
				return;
		}
	}
	if (LinkedStackBelow.IsValid())
	{
		LinkedStackBelow->ResetUserFocusRecursively();
	}
}

void UOUULayerStackWidget::AddRecursivelyToPlayerScreenFromRoot()
{
	GetLinkedStackRoot()->AddRecursivelyToPlayerScreen(0);
}

void UOUULayerStackWidget::AddRecursivelyToPlayerScreen(int32 ZOrder)
{
	RemoveFromViewport();
	AddToPlayerScreen(ZOrder);
	if (LinkedStackAbove.IsValid())
	{
		LinkedStackAbove->AddRecursivelyToPlayerScreen(ZOrder + 100);
	}
}
