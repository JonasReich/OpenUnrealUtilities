// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"

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
	 * @returns if the exit condition was set / iteration was canceled
	 */
	template<class WidgetClass>
	bool ForEachWidgetAndDescendants(WidgetClass* RootWidget, TFunctionRef<bool(WidgetClass*)> Predicate);

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
	bool OPENUNREALUTILITIES_API IsFocusable(const UWidget* Widget);

	/**
	 * Check if a widget is focusable, clickable or otherwise marked as "interactable" (see SWidget).
	 * Works both for UUserWidgets and native UWidgets.
	 */
	bool OPENUNREALUTILITIES_API IsInputVisible(const UWidget* Widget);

	/**
	 * Perform IsInputVisible check on all widgets in the widget tree underneath the specified widget.
	 * Traverses the widget tree using ForEachWidgetAndDescendants(), so the same rules apply.
	 * Stops search as soon as the first InputVisible widget is found.
	 */
	bool OPENUNREALUTILITIES_API HasAnyInputVisibleDescendantsIncludingSelf(const UWidget* Widget);

	//////////////////////////////////////////////////////////////////////////

	// Explicit instantiations of the widget tree iteration templates above
	template bool OPENUNREALUTILITIES_API ForEachWidget<UWidget>(UWidget* RootWidget, TFunctionRef<bool(UWidget*)> Predicate);
	template bool OPENUNREALUTILITIES_API ForEachWidget<const UWidget>(const UWidget* RootWidget, TFunctionRef<bool(const UWidget*)> Predicate);
	template bool OPENUNREALUTILITIES_API ForEachWidgetAndDescendants<UWidget>(UWidget* RootWidget, TFunctionRef<bool(UWidget*)> Predicate);
	template bool OPENUNREALUTILITIES_API ForEachWidgetAndDescendants<const UWidget>(const UWidget* RootWidget, TFunctionRef<bool(const UWidget*)> Predicate);
	template bool OPENUNREALUTILITIES_API ForChildWidgets<UWidget>(UWidget* Widget, TFunctionRef<bool(UWidget*)> Predicate);
	template bool OPENUNREALUTILITIES_API ForChildWidgets<const UWidget>(const UWidget* Widget, TFunctionRef<bool(const UWidget*)> Predicate);
}
