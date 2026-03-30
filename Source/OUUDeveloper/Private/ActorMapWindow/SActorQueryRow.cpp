// Copyright (c) 2026 Jonas Reich & Contributors

#include "SActorQueryRow.h"

#include "Widgets/Colors/SColorBlock.h"

namespace OUU::Developer::ActorMapWindow
{
	//------------------------------------------------------------------------
	// SActorQueryRow
	//------------------------------------------------------------------------

	void SActorQueryRow::Construct(
		const FArguments& InArgs,
		const TSharedRef<STableViewBase>& InOwnerTableView,
		const TSharedPtr<FOUUActorMapQuery>& InActorQuery)
	{
		ActorQuery = InActorQuery;
		ensure(ActorQuery.IsValid());

		ColumnSizeData = InArgs._ColumnSizeData;

		// clang-format off
		STableRow<TSharedPtr<FOUUActorMapQuery>>::Construct(
			STableRow<TSharedPtr<FOUUActorMapQuery>>::FArguments()
			.Padding(FMargin(4.f, 2.f))
			.Content()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().FillWidth(1.f)
					[
						SNew(SColorBlock)
						// Query color might change
						.Color_Lambda([this](){ return ActorQuery->QueryColor; })
					]
					+SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SButton).Text(INVTEXT("Delete")).OnClicked(InArgs._OnDeleteClicked)
					]
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Name Filter"),
						INVTEXT("Name string that must be contained within the actor names, for the actor to be "
							"included in the query."),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetNameFilter_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetNameFilter_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Name Regex Pattern"),
						INVTEXT("Regular expression pattern that must match to actor names, for the actor to be "
							"included in the query."),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetNameRegexPattern_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetNameRegexPattern_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					ColumnSizeData->MakeSimpleDetailsSplitter(
						INVTEXT("Class Filter"),
						INVTEXT("Name string that must match the actors class name or any of its "
							"super classes, for the actor to be included in the query."),
						SNew(SEditableTextBox)
						.Text(this, &SActorQueryRow::GetActorClassName_Text)
						.HintText(INVTEXT("<empty>"))
						.OnTextCommitted(this, &SActorQueryRow::SetActorClassName_Text)
					)
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					SNew(SBox)
					.Visibility_Lambda([=](){ return InArgs._EnableComponentFilters.Get() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
					[
						ColumnSizeData->MakeSimpleDetailsSplitter(
							INVTEXT("Component Class Filter"),
							INVTEXT("Name string that must match any of the actor component class names or any of their "
								"super classes, for the actor to be included in the query."),
							SNew(SEditableTextBox)
							.Text(this, &SActorQueryRow::GetComponentClassName_Text)
							.HintText(INVTEXT("<empty>"))
							.OnTextCommitted(this, &SActorQueryRow::SetComponentClassName_Text)
						)
					]
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					SNew(SBox)
					.Visibility_Lambda([=](){ return InArgs._EnableComponentFilters.Get() ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed; })
					[
						ColumnSizeData->MakeSimpleDetailsSplitter(
							INVTEXT("Gameplay Tag Query"),
							INVTEXT("Gameplay tag query. Must use fully qualified tag names that can be combined with ANY(), ALL() and NONE() operators.\ne.g. NONE(ANY(Foo.Bar, Foo.Baz), FooBar)"),
							SNew(SEditableTextBox)
							.Text(this, &SActorQueryRow::GetGameplayTagQueryString_Text)
							.HintText(INVTEXT("<empty>"))
							.OnTextCommitted(this, &SActorQueryRow::SetGameplayTagQueryString_Text)
						)
					]
				]
				+ SVerticalBox::Slot()
				.Padding(4.f, 2.f)
				[
					SNew(STextBlock).Text_Lambda([this]()
					{
						return ActorQuery.IsValid() && ActorQuery->CachedResult ? FText::Format(INVTEXT("Num Actors: {0}"), ActorQuery->CachedResult->Num()) :  INVTEXT("<invalid>");
					})
				]
			], InOwnerTableView);
		// clang-format on
	}

	void SActorQueryRow::SetGameplayTagQueryString_Text(const FText& Text, ETextCommit::Type)
	{
		FString TextAsString = Text.ToString();
		if (TextAsString != GameplayTagQueryString)
		{
			GameplayTagQueryString = TextAsString;
			if (ActorQuery.IsValid())
			{
				ActorQuery->ActorTagQuery = FGameplayTagQueryParser::ParseQuery(GameplayTagQueryString);
			}
		}
	}
} // namespace OUU::Developer::ActorMapWindow