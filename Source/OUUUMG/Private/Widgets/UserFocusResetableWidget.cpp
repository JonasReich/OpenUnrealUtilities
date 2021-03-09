// Copyright (c) 2021 Jonas Reich

#include "Widgets/UserFocusResetableWidget.h"
#include "Templates/InterfaceUtils.h"

bool IUserFocusResetableWidget::ResetUserFocus_Implementation()
{
	return false;
}

bool IUserFocusResetableWidget::TryResetUserFocusTo(UWidget* W)
{
	if (IsValidInterface<IUserFocusResetableWidget>(W))
	{
		return CALL_INTERFACE(IUserFocusResetableWidget, ResetUserFocus, W);
	}
	else
	{
		return false;
	}
}
