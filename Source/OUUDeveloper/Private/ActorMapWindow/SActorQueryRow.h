// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "ActorMapWindow/OUUActorMapQuery.h"
#include "ActorMapWindow/OUUActorMapWindow_TabSpawner.h"
#include "Slate/SplitterColumnSizeData.h"
#include "Widgets/Views/STableRow.h"

namespace OUU::Developer::ActorMapWindow
{
	namespace Private
	{
		extern FText GInvalidText;
	}

	//------------------------------------------------------------------------
	// SActorQueryRow
	//------------------------------------------------------------------------

	/** Slate widget for entries of a list of actor queries. */
	class SActorQueryRow : public STableRow<TSharedPtr<FOUUActorMapQuery>>
	{
	public:
		SLATE_BEGIN_ARGS(SActorQueryRow)
			{
			}

			SLATE_ARGUMENT(FSplitterColumnSizeData*, ColumnSizeData)
			SLATE_EVENT(FOnClicked, OnDeleteClicked)
			// Called whenever one of the filter fields is committed with a new value.
			SLATE_EVENT(FSimpleDelegate, OnQueryChanged)
			SLATE_ATTRIBUTE(bool, EnableComponentFilters)
		SLATE_END_ARGS()

		void Construct(
			const FArguments& InArgs,
			const TSharedRef<STableViewBase>& InOwnerTableView,
			const TSharedPtr<FOUUActorMapQuery>& InActorQuery);

	private:
		TSharedPtr<FOUUActorMapQuery> ActorQuery;
		FSplitterColumnSizeData* ColumnSizeData = nullptr;
		FString GameplayTagQueryString;
		FSimpleDelegate OnQueryChanged;

// Text boxes also commit when they lose focus, so the setters compare before they notify.
#define DEFINE_ACTOR_MAP_TEXT_ACCESSOR(Property)                                                                       \
	FORCEINLINE FText Get##Property##_Text() const                                                                     \
	{                                                                                                                  \
		return ActorQuery.IsValid() ? FText::FromString(ActorQuery->Property) : INVTEXT("<invalid>");                  \
	}                                                                                                                  \
	FORCEINLINE void Set##Property##_Text(const FText& Text, ETextCommit::Type) const                                  \
	{                                                                                                                  \
		if (ActorQuery.IsValid() == false)                                                                             \
			return;                                                                                                    \
		FString NewValue = Text.ToString();                                                                            \
		if (NewValue.Equals(ActorQuery->Property, ESearchCase::CaseSensitive))                                         \
			return;                                                                                                    \
		ActorQuery->Property = MoveTemp(NewValue);                                                                     \
		OnQueryChanged.ExecuteIfBound();                                                                               \
	}
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(NameFilter);
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(NameRegexPattern);
		FORCEINLINE FText GetActorClassName_Text() const
		{
			return ActorQuery.IsValid() ? FText::FromString(ActorQuery->ActorClassName) : INVTEXT("<invalid>");
		}
		FORCEINLINE void SetActorClassName_Text(const FText& Text, ETextCommit::Type) const
		{
			if (ActorQuery.IsValid() == false)
				return;

			FString NewValue = Text.ToString();
			if (NewValue.Equals(ActorQuery->ActorClassName, ESearchCase::CaseSensitive))
				return;

			ActorQuery->ActorClassName = MoveTemp(NewValue);
			ActorQuery->ResolvedActorClass = nullptr;
			ActorQuery->bClassNotResolved = true;
			OnQueryChanged.ExecuteIfBound();
		};
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(ComponentClassName);
#undef DEFINE_ACTOR_MAP_TEXT_ACCESSOR

		FORCEINLINE FText GetGameplayTagQueryString_Text() const
		{
			return ActorQuery.IsValid() ? FText::FromString(GameplayTagQueryString) : INVTEXT("<invalid>");
		}
		void SetGameplayTagQueryString_Text(const FText& Text, ETextCommit::Type);
	};
} // namespace OUU::Developer::ActorMapWindow