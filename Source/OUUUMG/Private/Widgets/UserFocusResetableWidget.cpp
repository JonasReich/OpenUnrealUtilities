// Copyright (c) 2022 Jonas Reich

#include "Widgets/UserFocusResetableWidget.h"

#include "Templates/InterfaceUtils.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS

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

PRAGMA_ENABLE_DEPRECATION_WARNINGS
