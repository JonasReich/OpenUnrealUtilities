// Copyright (c) 2024 Jonas Reich & Contributors

#include "Credits/OUUCreditsWidget.h"

#include "LogOpenUnrealUtilities.h"

//---------------------------------------------------------------------------------------------------------------------
// TSOUUCreditsList
//---------------------------------------------------------------------------------------------------------------------
template <typename ElementType>
void TSOUUCreditsList<ElementType>::Construct(const FArguments& InArgs)
{
	SListView<TSharedPtr<FOUUCreditsBlockListItem>>::Construct(InArgs);
	TimerHandle = RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SOUUCreditsList::RollCredits));
}

template <typename ElementType>
EActiveTimerReturnType TSOUUCreditsList<ElementType>::RollCredits(double InCurrentTime, float InDeltaTime)
{
	const float NewPixelOffset = ScrollSpeedPixelsPerSecond * InDeltaTime;
	ScrollBy(GetCachedGeometry(), NewPixelOffset, EAllowOverscroll::No);
	return EActiveTimerReturnType::Continue;
}

//---------------------------------------------------------------------------------------------------------------------
// UOUUCreditsWidget
//---------------------------------------------------------------------------------------------------------------------
UOUUCreditsWidget::UOUUCreditsWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	WheelScrollMultiplier = 0.f;
	AllowOverscroll = false;
	bEnableRightClickScrolling = false;
	bAllowDragging = false;
}

void UOUUCreditsWidget::SetScrollSpeedPixelsPerSecond(float InScrollSpeedPixelsPerSecond)
{
	ScrollSpeedPixelsPerSecond = InScrollSpeedPixelsPerSecond;

	if (auto Widget = StaticCastSharedPtr<SOUUCreditsList>(TakeWidget().ToSharedPtr()))
	{
		Widget->ScrollSpeedPixelsPerSecond = InScrollSpeedPixelsPerSecond;
	}
}

void UOUUCreditsWidget::ResetCreditScroll()
{
	SetScrollOffset(0);
	bReachedEndOfCredits = false;
}

UUserWidget& UOUUCreditsWidget::OnGenerateEntryWidgetInternal(
	TSharedPtr<FOUUCreditsBlockListItem> Item,
	TSubclassOf<UUserWidget> DesiredEntryClass,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	if (DesiredEntryClass->IsChildOf<UOUUCreditsWidgetBlock>() == false)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Error,
			TEXT("Expected entry class for %s to be a child class of UOUUCreditsWidgetBlock"),
			*GetNameSafe(this));
		return GenerateTypedEntry(DesiredEntryClass, OwnerTable);
	}

	const auto TypedEntryClass = TSubclassOf<UOUUCreditsWidgetBlock>(DesiredEntryClass);
	auto& NewEntry = GenerateTypedEntry<UOUUCreditsWidgetBlock, SObjectTableRow<TSharedPtr<FOUUCreditsBlockListItem>>>(
		TypedEntryClass,
		OwnerTable);
	NewEntry.SetCreditsBlockData(static_cast<FOUUCreditsBlock>(*Item.Get()));
	if (ListItems[0] == Item)
	{
		NewEntry.SetIsFirstItem();
	}
	else if (ListItems.Last() == Item)
	{
		NewEntry.SetIsLastItem();
	}
	return NewEntry;
}

void UOUUCreditsWidget::SetCredits(const FOUUCredits& Credits)
{
	for (auto& Block : Credits.Blocks)
	{
		auto BlockCopy = MakeShared<FOUUCreditsBlockListItem>(Block);
		BlockCopy->SortPeople(PeopleSortMode);
		ListItems.Add(BlockCopy);
	}

	RequestRefresh();
}

void UOUUCreditsWidget::SetPeopleSortMode(EOUUCreditsSortMode SortMode)
{
	if (SortMode == PeopleSortMode)
	{
		return;
	}

	if (SortMode == EOUUCreditsSortMode::KeepInputOrder)
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("People sorting in credits view is destructive. Cannot set sort mode to KeepInputOrder after the "
				 "entries were already sorted by a different mode."));
		return;
	}

	PeopleSortMode = SortMode;
	for (const auto& BlockPtr : ListItems)
	{
		if (BlockPtr.IsValid())
		{
			BlockPtr->SortPeople(PeopleSortMode);
		}
	}
	RequestRefresh();
}

TSharedRef<STableViewBase> UOUUCreditsWidget::RebuildListWidget()
{
	FListViewConstructArgs Args;
	Args.bAllowFocus = false;
	Args.SelectionMode = ESelectionMode::None;
	Args.bClearSelectionOnClick = false;
	Args.ConsumeMouseWheel = EConsumeMouseWheel::WhenScrollingPossible;
	Args.bReturnFocusToSelection = false;
	Args.Orientation = Orient_Vertical;

	CreditsListView = ConstructListView<TSOUUCreditsList>(this, ListItems, Args);
	CreditsListView->ScrollSpeedPixelsPerSecond = ScrollSpeedPixelsPerSecond;

	auto TableView = StaticCastSharedRef<STableViewBase>(CreditsListView.ToSharedRef());
	TableView->SetScrollbarVisibility(EVisibility::Collapsed);

	if (OnFinishedScrollingEvent.IsBoundToObject(this) == false)
	{
		OnFinishedScrollingEvent.AddUObject(this, &UOUUCreditsWidget::HandleOnFinishedScrolling);
	}

	return TableView;
}

void UOUUCreditsWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	CreditsListView.Reset();
}

void UOUUCreditsWidget::HandleOnFinishedScrolling()
{
	if (auto Widget = StaticCastSharedPtr<SOUUCreditsList>(TakeWidget().ToSharedPtr()))
	{
		if (bReachedEndOfCredits == false && Widget->IsAtEndOfCredits())
		{
			bReachedEndOfCredits = true;
			OnEndOfCreditsReached.Broadcast();
		}
	}
}
