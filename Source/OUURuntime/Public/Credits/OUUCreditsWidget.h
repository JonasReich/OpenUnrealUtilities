// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "CommonNativeListItem.h"
#include "Components/ListViewBase.h"
#include "Components/Widget.h"
#include "OUUCredits.h"

#include "OUUCreditsWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndOfCreditsReached);

struct FOUUCreditsBlockListItem : public FOUUCreditsBlock, public FCommonNativeListItem
{
	DERIVED_LIST_ITEM(FOUUCreditsBlockListItem, FCommonNativeListItem);

public:
	FOUUCreditsBlockListItem() = default;
	FOUUCreditsBlockListItem(const FOUUCreditsBlock& SuperInst) { *ImplicitConv<FOUUCreditsBlock*>(this) = SuperInst; }
};

template <typename ElementType>
class TSOUUCreditsList : public SListView<TSharedPtr<FOUUCreditsBlockListItem>>
{
public:
	static_assert(
		std::is_same_v<ElementType, TSharedPtr<FOUUCreditsBlockListItem>>,
		"ElementType must be TSharedRef<FOUUCreditsBlock>");

	TWeakPtr<FActiveTimerHandle> TimerHandle;
	float ScrollSpeedPixelsPerSecond = 50.0f;

	void Construct(const FArguments& InArgs);
	EActiveTimerReturnType RollCredits(double InCurrentTime, float InDeltaTime);

	bool IsAtEndOfCredits() const
	{
		return (ScrollBar->DistanceFromBottom() < SMALL_NUMBER);
	}
};

using SOUUCreditsList = TSOUUCreditsList<TSharedPtr<FOUUCreditsBlockListItem>>;

UCLASS(BlueprintType)
class UOUUCreditsWidgetBlock : public UUserWidget, public IUserListEntry
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintImplementableEvent)
	void SetCreditsBlockData(FOUUCreditsBlock NewBlockData);

	// Set that this is the first item. Probably needs more spacing at the beginning
	UFUNCTION(BlueprintImplementableEvent)
	void SetIsFirstItem();

	// Set that this is the last item. Probably needs more spacing at the end.
	UFUNCTION(BlueprintImplementableEvent)
	void SetIsLastItem();
};

UCLASS(BlueprintType)
class UOUUCreditsWidget : public UListViewBase, public ITypedUMGListView<TSharedPtr<FOUUCreditsBlockListItem>>
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_UMG_LIST(TSharedPtr<FOUUCreditsBlockListItem>, CreditsListView)

public:
	UFUNCTION(BlueprintCallable)
	void SetCredits(const FOUUCredits& Credits);

	UFUNCTION(BlueprintCallable)
	void SetPeopleSortMode(EOUUCreditsSortMode SortMode);

	UFUNCTION(BlueprintCallable)
	void SetScrollSpeedPixelsPerSecond(float ScrollSpeed);

	UFUNCTION(BlueprintCallable)
	void ResetCreditScroll();

protected:
	// - UListViewBase
	UUserWidget& OnGenerateEntryWidgetInternal(
		TSharedPtr<FOUUCreditsBlockListItem> Item,
		TSubclassOf<UUserWidget> DesiredEntryClass,
		const TSharedRef<STableViewBase>& OwnerTable) override;
	TSharedRef<STableViewBase> RebuildListWidget() override;

#if WITH_EDITOR
	void OnRefreshDesignerItems() override
	{
		RefreshDesignerItems<TSharedPtr<FOUUCreditsBlockListItem>>(ListItems, []() {
			return MakeShared<FOUUCreditsBlockListItem>();
		});
	}
#endif

	// - UWidget
	void ReleaseSlateResources(bool bReleaseChildren) override;
	// --

private:
	UFUNCTION()
	void HandleOnFinishedScrolling();

public:
	UPROPERTY(BlueprintAssignable)
	FOnEndOfCreditsReached OnEndOfCreditsReached;

protected:
	TArray<TSharedPtr<FOUUCreditsBlockListItem>> ListItems;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	EOUUCreditsSortMode PeopleSortMode;

	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float ScrollSpeedPixelsPerSecond;


private:
	bool bReachedEndOfCredits = false;
	TSharedPtr<SOUUCreditsList> CreditsListView;
};
