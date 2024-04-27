// Copyright (c) 2023 Jonas Reich & Contributors

#include "UMG/UMGUtils.h"

#include "LogOpenUnrealUtilities.h"
#include "Widgets/SWidget.h"

namespace OUU::Runtime::UMGUtils
{
	template <class WidgetClass>
	bool ForEachWidget(WidgetClass* RootWidget, TFunctionRef<bool(WidgetClass*)> Predicate)
	{
		if (RootWidget)
		{
			if (Predicate(RootWidget))
				return true;

			if (const UUserWidget* UserWidgetChild = Cast<UUserWidget>(RootWidget))
			{
				if (UserWidgetChild->WidgetTree)
				{
					if (Predicate(UserWidgetChild->WidgetTree->RootWidget))
						return true;

					if (UMGUtils::ForChildWidgets<WidgetClass>(UserWidgetChild->WidgetTree->RootWidget, Predicate))
						return true;
				}
			}

			if (UMGUtils::ForChildWidgets<WidgetClass>(RootWidget, Predicate))
				return true;
		}
		return false;
	}

	template <class WidgetClass>
	bool ForEachWidgetAndDescendants(
		WidgetClass* RootWidget,
		bool bIncludeRootWidget,
		TFunctionRef<bool(WidgetClass*)> Predicate)
	{
		if (!RootWidget)
			return false;

		if (bIncludeRootWidget)
		{
			if (Predicate(RootWidget))
				return true;
		}

		if (const UUserWidget* UserWidgetChild = Cast<UUserWidget>(RootWidget))
		{
			if (UserWidgetChild->WidgetTree)
			{
				if (UMGUtils::ForEachWidgetAndDescendants<WidgetClass>(
						UserWidgetChild->WidgetTree->RootWidget,
						true,
						Predicate))
					return true;
			}
		}

		// Search for any named slot with content that we need to dive into.
		if (const INamedSlotInterface* NamedSlotHost = Cast<INamedSlotInterface>(RootWidget))
		{
			TArray<FName> SlotNames;
			NamedSlotHost->GetSlotNames(SlotNames);

			for (const FName& SlotName : SlotNames)
			{
				if (WidgetClass* SlotContent = NamedSlotHost->GetContentForSlot(SlotName))
				{
					if (UMGUtils::ForEachWidgetAndDescendants<WidgetClass>(SlotContent, true, Predicate))
						return true;
				}
			}
		}

		// Search standard children.
		if (const UPanelWidget* PanelParent = Cast<UPanelWidget>(RootWidget))
		{
			for (int32 ChildIndex = 0; ChildIndex < PanelParent->GetChildrenCount(); ChildIndex++)
			{
				if (WidgetClass* ChildWidget = PanelParent->GetChildAt(ChildIndex))
				{
					if (UMGUtils::ForEachWidgetAndDescendants<WidgetClass>(ChildWidget, true, Predicate))
						return true;
				}
			}
		}

		return false;
	}

	template <class WidgetClass>
	bool ForChildWidgets(WidgetClass* Widget, TFunctionRef<bool(WidgetClass*)> Predicate)
	{
		// Search for any named slot with content that we need to dive into.
		if (const INamedSlotInterface* NamedSlotHost = Cast<INamedSlotInterface>(Widget))
		{
			TArray<FName> SlotNames;
			NamedSlotHost->GetSlotNames(SlotNames);

			for (const FName& SlotName : SlotNames)
			{
				if (WidgetClass* SlotContent = NamedSlotHost->GetContentForSlot(SlotName))
				{
					if (Predicate(SlotContent))
						return true;

					if (UMGUtils::ForChildWidgets<WidgetClass>(SlotContent, Predicate))
						return true;
				}
			}
		}

		// Search standard children.
		if (const UPanelWidget* PanelParent = Cast<UPanelWidget>(Widget))
		{
			for (int32 ChildIndex = 0; ChildIndex < PanelParent->GetChildrenCount(); ChildIndex++)
			{
				if (WidgetClass* ChildWidget = PanelParent->GetChildAt(ChildIndex))
				{
					if (Predicate(ChildWidget))
						return true;

					if (UMGUtils::ForChildWidgets<WidgetClass>(ChildWidget, Predicate))
						return true;
				}
			}
		}

		return false;
	}

	bool IsFocusable(const UWidget* Widget)
	{
		const TSharedPtr<SWidget> SlateWidget = Widget->GetCachedWidget();
		if (!SlateWidget.IsValid())
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Warning,
				TEXT("UMGUtils::IsFocusable() could not determine focusability of widget %s "
					 "because the cached slate widget was invalid"),
				*Widget->GetName());
			return false;
		}

		return SlateWidget->SupportsKeyboardFocus();
	}

	bool IsInputVisible(const UWidget* UmgWidget)
	{
		const TSharedPtr<SWidget> Widget = UmgWidget->GetCachedWidget();
		if (!Widget.IsValid())
			return false;

		if (Widget->IsEnabled() && Widget->GetVisibility().IsVisible())
		{
			if (Widget->IsInteractable())
				return true;

			if (Widget->SupportsKeyboardFocus())
				return true;
		}

		if (Widget->GetVisibility().IsHitTestVisible())
			return true;

		return false;
	}

	bool HasInputVisibleDescendantsIncludingSelf(const UWidget* Widget)
	{
		return UMGUtils::ForEachWidgetAndDescendants<const UWidget>(Widget, true, [&](const UWidget* W) -> bool {
			return UMGUtils::IsInputVisible(W);
		});
	}

	bool HasInputVisibleDescendantsExcludingSelf(const UWidget* Widget)
	{
		return UMGUtils::ForEachWidgetAndDescendants<const UWidget>(Widget, false, [&](const UWidget* W) -> bool {
			return UMGUtils::IsInputVisible(W);
		});
	}

	UWidget* GetFirstFocusableDescendantIncludingSelf(UWidget* Widget)
	{
		UWidget* Result = nullptr;
		UMGUtils::ForEachWidgetAndDescendants<UWidget>(Widget, true, [&](UWidget* W) -> bool {
			if (UMGUtils::IsFocusable(W))
			{
				Result = W;
				return true;
			}
			return false;
		});
		return Result;
	}

	//////////////////////////////////////////////////////////////////////////

	// Explicit instantiations of the widget tree iteration templates above
	template bool OUURUNTIME_API ForEachWidget<UWidget>(UWidget* RootWidget, TFunctionRef<bool(UWidget*)> Predicate);
	template bool OUURUNTIME_API
		ForEachWidget<const UWidget>(const UWidget* RootWidget, TFunctionRef<bool(const UWidget*)> Predicate);
	template bool OUURUNTIME_API ForEachWidgetAndDescendants<UWidget>(
		UWidget* RootWidget,
		bool bIncludeRootWidget,
		TFunctionRef<bool(UWidget*)> Predicate);
	template bool OUURUNTIME_API ForEachWidgetAndDescendants<const UWidget>(
		const UWidget* RootWidget,
		bool bIncludeRootWidget,
		TFunctionRef<bool(const UWidget*)> Predicate);
	template bool OUURUNTIME_API ForChildWidgets<UWidget>(UWidget* Widget, TFunctionRef<bool(UWidget*)> Predicate);
	template bool OUURUNTIME_API
		ForChildWidgets<const UWidget>(const UWidget* Widget, TFunctionRef<bool(const UWidget*)> Predicate);

} // namespace OUU::Runtime::UMGUtils
