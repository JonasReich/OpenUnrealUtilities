// Copyright (c) 2021 Jonas Reich

#include "UMG/LayerWidget.h"
#include "Widgets/SWidget.h"
#include "Widgets/SViewport.h"
#include "Framework/Application/SlateApplication.h"
#include "OUURuntimeModule.h"
#include "UMG/UMGUtils.h"
#include "UMG/UMGInputBinding.h"

void UOUULayerWidget::UpdateLayer(const UOUULayerWidget* LayerAbove)
{
	if (IsValid(LayerAbove))
	{
		bHasConcealingLayerAbove = (LayerAbove->IsActivelyConcealing() || LayerAbove->bHasConcealingLayerAbove);
	}
	else
	{
		bHasConcealingLayerAbove = false;
	}

	CheckLayerVisibility();
	SetVisibility(GetDesiredVisibility());
}

bool UOUULayerWidget::IsActivelyConcealing() const
{
	return bConcealLayersBelow && IsVisible();
}

ESlateVisibility UOUULayerWidget::GetDesiredVisibility() const
{
	if (!IsConcealed() && bIsLayerVisible)
	{
		return IsLayerInputVisible() ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible;
	}
	
	return ESlateVisibility::Collapsed;
}

UUMGInputActionBindingStack* UOUULayerWidget::GetInputActionBindingStack()
{
	if (!bAllowInput)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("GetInputActionBindingStack was called on Layer Widget '%s' that is not marked as bAllowInput!"));
		return nullptr;
	}
	if (!IsValid(InputActionBindingStack))
	{
		InputActionBindingStack = UUMGInputActionBindingStack::CreateUMGInputActionBindingStack(this);
	}
	return InputActionBindingStack;
}

void UOUULayerWidget::NativeOnFocusChanging(const FWeakWidgetPath& PreviousFocusPath, const FWidgetPath& NewWidgetPath, const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusChanging(PreviousFocusPath, NewWidgetPath, InFocusEvent);
	auto SafeWidget = TakeWidget();

	if (PreviousFocusPath.ContainsWidget(SafeWidget))
		LastValidFocusPath = PreviousFocusPath;
	if (!NewWidgetPath.ContainsWidget(SafeWidget))
	{
		// probable loss of focus on the layer
		// we want to check if the new focus path is outside of the game viewport, because that is a common scenario in PIE
		// during which we do NOT want the focus to be constantly reset
		if (DoesPathContainGameViewport(NewWidgetPath))
		{
			OnRequestResetFocusFromTopLayer.Broadcast();
		}
	}
}

bool UOUULayerWidget::DoesPathContainGameViewport(const FWidgetPath& NewWidgetPath)
{
#if WITH_EDITOR
	// In the editor we need to actually perform the check:
	TSharedPtr<SWidget> GameViewport = StaticCastSharedPtr<SWidget>(FSlateApplication::Get().GetGameViewport());
	return GameViewport.IsValid() && NewWidgetPath.ContainsWidget(GameViewport.ToSharedRef());
#else
	// Assume the game viewport is always part of a focus path in non-editor builds as there should only be the game:
	return true;
#endif
}

bool UOUULayerWidget::IsConcealed() const
{
	return bHasConcealingLayerAbove && bMayBeConcealedFromAbove;
}

void UOUULayerWidget::CheckLayerVisibility()
{
	if (bIsFocusable)
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Layer widgets themselves must not be focusable! Made layer '%s' not focusable..."), *GetName());
		bIsFocusable = false;
	}

	if (!IsConcealed())
	{
		bIsLayerVisible = false;
		bIsLayerInputVisible = false;

		UMGUtils::ForEachWidgetAndDescendants<const UWidget>(this, false, [&](const UWidget* W) -> bool
		{
			bIsLayerVisible |= W->IsVisible();
			bIsLayerInputVisible = bAllowInput && UMGUtils::IsInputVisible(W);
			
			// break when we have all the info we need
			return bIsLayerInputVisible || (!bAllowInput && bIsLayerVisible);
		});
	}
	else
	{
		bIsLayerVisible = false;
		bIsLayerInputVisible = false;
	}
}

bool UOUULayerWidget::IsLayerInputVisible() const
{
	return bIsLayerInputVisible;
}

bool UOUULayerWidget::ResetUserFocus_Implementation()
{
	if (LastValidFocusPath.IsValid())
	{
		FWidgetPath WidgetPath = LastValidFocusPath.ToWidgetPath();
		int32 FocusableWidgetIdx = WidgetPath.Widgets.FindLastByPredicate([](const FArrangedWidget& WidgetPathElement) -> bool
		{
			return WidgetPathElement.Widget->SupportsKeyboardFocus();
		});
		if (FocusableWidgetIdx != INDEX_NONE)
		{
			TSharedPtr<SWidget> SafeWidget = WidgetPath.Widgets[FocusableWidgetIdx].Widget;
			if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
			{
				TOptional<int32> UserIndex = FSlateApplication::Get().GetUserIndexForController(LocalPlayer->GetControllerId());
				if (UserIndex.IsSet())
				{
					FReply& DelayedSlateOperations = LocalPlayer->GetSlateOperations();
					return FSlateApplication::Get().SetUserFocus(UserIndex.GetValue(), SafeWidget);
				}
			}
		}
	}

	return ResetUserFocus_TemplateImplementation();
}
