// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UMG/UMGUtils.h"
#include "UserFocusResetableWidget.generated.h"

UINTERFACE()
class OUUUMG_API UUserFocusResetableWidget : public UInterface
{
	GENERATED_BODY()
};

/**
 * Base widget that shows how you can integrate UUMGInputActionBindingStack and how
 * to implement the IUserFocusResettableWidget interface
 */
class OUUUMG_API IUserFocusResetableWidget : public IInterface
{
	GENERATED_BODY()
public:
	static bool TryResetUserFocusTo(UWidget* W);

protected:
	/**
	 * Reset the user focus to the widget implementing this interface or any of its child widgets.
	 * @returns if the user focus was set by the implementation and a widget in the tree under the target widget now has user focus.
	 * Should return true even if the focus remains on the same widget as before.
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	bool ResetUserFocus();

	virtual bool ResetUserFocus_Implementation();
};

/** Template implementation of IUserFocusResetableWidget for UUserWidgets */
template<typename ImplementingType>
class TUserFocusResetableWidget_Impl
{
public:
	// Call this function in the C++ implementation of IUserFocusResetableWidget::ResetUserFocus_Implementation()
	bool ResetUserFocus_TemplateImplementation()
	{
		static_assert(TIsDerivedFrom<ImplementingType, UUserWidget>::Value,
			"ImplementingType must be a UUserWidget child class");
		static_assert(TIsDerivedFrom<ImplementingType, IUserFocusResetableWidget>::Value,
			"ImplementingType must implement interface class IUserFocusResetableWidget");
		static_assert(TIsDerivedFrom<ImplementingType, TUserFocusResetableWidget_Impl<ImplementingType>>::Value,
			"ImplementingType must be the class that derives from the template");

		ImplementingType* ThisAsImplementingType = StaticCast<ImplementingType*>(this);
		return UMGUtils::ForEachWidgetAndDescendants<UWidget>(ThisAsImplementingType, true, [&](UWidget* W) -> bool
		{
			// Skip the initial widget implementing the interface, otherwise we end up in an endless loop
			if (W != ThisAsImplementingType)
			{
				if (IUserFocusResetableWidget::TryResetUserFocusTo(W))
					return true;
			}
			
			if (UMGUtils::IsFocusable(W))
			{
				W->SetUserFocus(ThisAsImplementingType->GetOwningPlayer());
				return true;
			}

			return false;
		});

		return false;
	}
};
