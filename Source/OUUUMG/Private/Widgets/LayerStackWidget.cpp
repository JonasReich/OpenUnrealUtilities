// Copyright(c) 2022 Jonas Reich

#include "Widgets/LayerStackWidget.h"

#include "Components/Overlay.h"
#include "LogOpenUnrealUtilities.h"
#include "Templates/CastObjectRange.h"
#include "Widgets/LayerWidget.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

UOUULayerStackWidget_DEPRECATED::UOUULayerStackWidget_DEPRECATED(const FObjectInitializer& ObjectInitializer) :
	Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UOUULayerStackWidget_DEPRECATED::StartNewLinkedStack()
{
	bAddedCorrectly = true;
	AddRecursivelyToPlayerScreenFromRoot();
}

void UOUULayerStackWidget_DEPRECATED::InsertToLinkedStackAbove(UOUULayerStackWidget_DEPRECATED* OtherStack)
{
	if (!IsValid(OtherStack))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Cannot insert stack %s above an invalid stack"), *GetName());
		return;
	}

	InsertBetween(OtherStack->GetStackAbove(), OtherStack);
}

void UOUULayerStackWidget_DEPRECATED::InsertToLinkedStackBelow(UOUULayerStackWidget_DEPRECATED* OtherStack)
{
	if (!IsValid(OtherStack))
	{
		UE_LOG(LogOpenUnrealUtilities, Error, TEXT("Cannot insert stack %s below an invalid stack"), *GetName());
		return;
	}

	InsertBetween(OtherStack, OtherStack->GetStackBelow());
}

void UOUULayerStackWidget_DEPRECATED::RemoveFromLinkedStack()
{
	if (UOUULayerStackWidget_DEPRECATED* StackAbove = GetStackAbove())
	{
		StackAbove->LinkedStackBelow = LinkedStackBelow;
	}

	if (UOUULayerStackWidget_DEPRECATED* StackBelow = GetStackBelow())
	{
		StackBelow->LinkedStackAbove = LinkedStackAbove;
	}

	LinkedStackAbove.Reset();
	LinkedStackBelow.Reset();
}

bool UOUULayerStackWidget_DEPRECATED::IsLinkedStackHead() const
{
	if (!bAddedCorrectly)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("LayerStackWidget was not initialized correctly and is not part of a linked stack! "
				 "You must call StartNewLinkedStack() to create a new linked stack or one of the insert functions to "
				 "add a LayerStackWidget to "
				 "an existing linked stack before calling any of the other member functions!"));
	}

	return LinkedStackAbove.IsValid() == false;
}

bool UOUULayerStackWidget_DEPRECATED::IsLinkedStackRoot() const
{
	if (!bAddedCorrectly)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("LayerStackWidget was not initialized correctly and is not part of a linked stack! "
				 "You must call StartNewLinkedStack() to create a new linked stack or one of the insert functions to "
				 "add a LayerStackWidget to "
				 "an existing linked stack before calling any of the other member functions!"));
	}

	return LinkedStackBelow.IsValid() == false;
}

UOUULayerStackWidget_DEPRECATED* UOUULayerStackWidget_DEPRECATED::GetStackAbove() const
{
	if (LinkedStackAbove.IsValid())
		return LinkedStackAbove.Get();
	return nullptr;
}

UOUULayerStackWidget_DEPRECATED* UOUULayerStackWidget_DEPRECATED::GetStackBelow() const
{
	if (LinkedStackBelow.IsValid())
		return LinkedStackBelow.Get();
	return nullptr;
}

UOUULayerStackWidget_DEPRECATED* UOUULayerStackWidget_DEPRECATED::GetLinkedStackHead()
{
	return IsLinkedStackHead() ? this : LinkedStackAbove->GetLinkedStackHead();
}

UOUULayerStackWidget_DEPRECATED* UOUULayerStackWidget_DEPRECATED::GetLinkedStackRoot()
{
	return IsLinkedStackRoot() ? this : LinkedStackBelow->GetLinkedStackRoot();
}

void UOUULayerStackWidget_DEPRECATED::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (!IsValid(WidgetStack))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Error,
			TEXT("WidgetStack overlay widget is not bound in NativeOnInitialized. "
				 "This will prevents all of the stack functionality from functioning correctly!"));
		return;
	}

	int32 NumLayersFound = 0;
	for (UOUULayerWidget_DEPRECATED* Layer : CastObjectRange<UOUULayerWidget_DEPRECATED>(WidgetStack->GetAllChildren()))
	{
		NumLayersFound++;
		Layer->OnRequestResetFocusFromTopLayer
			.AddDynamic(this, &UOUULayerStackWidget_DEPRECATED::HandleRequestResetFocusFromTopLayer);
	}

	UE_CLOG(
		NumLayersFound == 0,
		LogOpenUnrealUtilities,
		Warning,
		TEXT("WidgetStack overlay widget does not contain any UOUULayerWidget_DEPRECATED children"));
}

void UOUULayerStackWidget_DEPRECATED::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (IsLinkedStackHead())
	{
		UpdateLayers(nullptr);
	}
}

void UOUULayerStackWidget_DEPRECATED::NativeDestruct()
{
	if (IsValid(WidgetStack))
	{
		for (UOUULayerWidget_DEPRECATED* Layer :
			 CastObjectRange<UOUULayerWidget_DEPRECATED>(WidgetStack->GetAllChildren()))
		{
			Layer->OnRequestResetFocusFromTopLayer
				.RemoveDynamic(this, &UOUULayerStackWidget_DEPRECATED::HandleRequestResetFocusFromTopLayer);
		}
	}
	Super::NativeDestruct();
}

void UOUULayerStackWidget_DEPRECATED::InsertBetween(
	UOUULayerStackWidget_DEPRECATED* NewAbove,
	UOUULayerStackWidget_DEPRECATED* NewBelow)
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

void UOUULayerStackWidget_DEPRECATED::UpdateLayers(UOUULayerWidget_DEPRECATED* BottomLayerFromStackAbove)
{
	UOUULayerWidget_DEPRECATED* LastLayer = BottomLayerFromStackAbove;
	if (IsValid(WidgetStack))
	{
		for (UOUULayerWidget_DEPRECATED* Layer :
			 CastObjectRange<UOUULayerWidget_DEPRECATED>(WidgetStack->GetAllChildren()))
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

void UOUULayerStackWidget_DEPRECATED::HandleRequestResetFocusFromTopLayer()
{
	// forward to linked stack head.
	// we do not bind this delegate to linked stack head directly to make it a bit easier to link in new stack widgets
	// after the fact
	GetLinkedStackHead()->ResetUserFocusRecursively();
}

void UOUULayerStackWidget_DEPRECATED::ResetUserFocusRecursively()
{
	if (IsValid(WidgetStack))
	{
		for (UOUULayerWidget_DEPRECATED* Layer :
			 CastObjectRange<UOUULayerWidget_DEPRECATED>(WidgetStack->GetAllChildren()))
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

void UOUULayerStackWidget_DEPRECATED::AddRecursivelyToPlayerScreenFromRoot()
{
	GetLinkedStackRoot()->AddRecursivelyToPlayerScreen(0);
}

void UOUULayerStackWidget_DEPRECATED::AddRecursivelyToPlayerScreen(int32 ZOrder)
{
	RemoveFromViewport();
	AddToPlayerScreen(ZOrder);
	if (LinkedStackAbove.IsValid())
	{
		LinkedStackAbove->AddRecursivelyToPlayerScreen(ZOrder + 100);
	}
}

PRAGMA_ENABLE_DEPRECATION_WARNINGS
