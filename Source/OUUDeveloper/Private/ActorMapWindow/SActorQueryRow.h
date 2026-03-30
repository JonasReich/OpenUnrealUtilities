// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "ActorMapWindow/OUUActorMapWindow_TabSpawner.h"
#include "Slate/SplitterColumnSizeData.h"

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

#define DEFINE_ACTOR_MAP_TEXT_ACCESSOR(Property)                                                                       \
	FORCEINLINE FText Get##Property##_Text() const                                                                     \
	{                                                                                                                  \
		return ActorQuery.IsValid() ? FText::FromString(ActorQuery->Property) : INVTEXT("<invalid>");                  \
	}                                                                                                                  \
	FORCEINLINE void Set##Property##_Text(const FText& Text, ETextCommit::Type) const                                  \
	{                                                                                                                  \
		if (ActorQuery.IsValid())                                                                                      \
		{                                                                                                              \
			ActorQuery->Property = Text.ToString();                                                                    \
		}                                                                                                              \
	}
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(NameFilter);
		DEFINE_ACTOR_MAP_TEXT_ACCESSOR(NameRegexPattern);
		FORCEINLINE FText GetActorClassName_Text() const
		{
			return ActorQuery.IsValid() ? FText::FromString(ActorQuery->ActorClassName) : INVTEXT("<invalid>");
		}
		FORCEINLINE void SetActorClassName_Text(const FText& Text, ETextCommit::Type) const
		{
			if (ActorQuery.IsValid())
			{
				ActorQuery->ActorClassName = Text.ToString();
				ActorQuery->ResolvedActorClass = nullptr;
				ActorQuery->bClassNotResolved = true;
			}
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