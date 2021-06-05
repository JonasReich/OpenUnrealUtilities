// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"

class UWidget;

namespace UMGUtils
{
	/**
	 * Execute a predicate on the widget and all widgets nested as Slot or Named Slot children.
	 * Does not jump from one UserWidget into another.
	 * Modeled after UWidgetTree::ForEachWidget, but the predicate returns a boolean break condition to stop the iteration.
	 * @returns if the exit condition was set / iteration was canceled
	 */
	template<class WidgetClass>
	bool ForEachWidget(WidgetClass* RootWidget, TFunctionRef<bool(WidgetClass*)> Predicate);

	/**
	 * Execute a predicate on the RootWidget and any widgets nested as Slot or Named Slot children.
	 * Also goes into other UUserWidgets and their children (so this potentially goes a lot deeper than ForEachWidget).
	 * Modeled after UWidgetTree::ForEachWidgetAndDescendants, but the predicate returns a boolean break condition
	 * @param RootWidget: The widget of which children should be iterated over
	 * @param bIncludeRootWidget: If the predicate should also be called on the root widget itself
	 * @param Predicate: The predicate to execute for the widgets
	 * @returns if the exit condition was set / iteration was canceled
	 */
	template<class WidgetClass> bool ForEachWidgetAndDescendants(WidgetClass* RootWidget, bool bIncludeRootWidget, TFunctionRef<bool(WidgetClass*)> Predicate);

	/**
	 * Execute a predicate on all widgets nested as Slot or Named Slot children under the RootWidget.
	 * Does not call the predicate on the RootWidget itself!
	 * Modeled after UWidgetTree::ForWidgetAndChildren (that also only checks children contrary to the name)
	 * but the predicate returns a boolean break condition
	 * @returns if the exit condition was set / iteration was canceled
	 */
	template<class WidgetClass>
	bool ForChildWidgets(WidgetClass* Widget, TFunctionRef<bool(WidgetClass*)> Predicate);

	/** Check if a widget is focusable. Works both for UUserWidgets and native UWidgets. */
	bool OUURUNTIME_API IsFocusable(const UWidget* Widget);

	/**
	 * Check if a widget is focusable, clickable or otherwise marked as "interactable" (see SWidget).
	 * Works both for UUserWidgets and native UWidgets.
	 */
	bool OUURUNTIME_API IsInputVisible(const UWidget* Widget);

	/**
	 * Check if a widget has input visible descendants or is input visible itself.
	 */
	bool OUURUNTIME_API HasInputVisibleDescendantsIncludingSelf(const UWidget* Widget);

	/**
	 * Check if a widget has input visible descendants.
	 * The check skips the initial target widget and only checks children!
	 */
	bool OUURUNTIME_API HasInputVisibleDescendantsExcludingSelf(const UWidget* Widget);

	/** @returns the first descendant in the widget tree that is focusable. May return nullptr */
	OUURUNTIME_API UWidget* GetFirstFocusableDescendantIncludingSelf(UWidget* Widget);

	//////////////////////////////////////////////////////////////////////////

	// Explicit instantiations of the widget tree iteration templates above
	extern template bool ForEachWidget<UWidget>(UWidget* RootWidget, TFunctionRef<bool(UWidget*)> Predicate);
	extern template bool ForEachWidget<const UWidget>(const UWidget* RootWidget, TFunctionRef<bool(const UWidget*)> Predicate);
	extern template bool ForEachWidgetAndDescendants<UWidget>(UWidget* RootWidget, bool bIncludeRootWidget, TFunctionRef<bool(UWidget*)> Predicate);
	extern template bool ForEachWidgetAndDescendants<const UWidget>(const UWidget* RootWidget, bool bIncludeRootWidget, TFunctionRef<bool(const UWidget*)> Predicate);
	extern template bool ForChildWidgets<UWidget>(UWidget* Widget, TFunctionRef<bool(UWidget*)> Predicate);
	extern template bool ForChildWidgets<const UWidget>(const UWidget* Widget, TFunctionRef<bool(const UWidget*)> Predicate);
}
