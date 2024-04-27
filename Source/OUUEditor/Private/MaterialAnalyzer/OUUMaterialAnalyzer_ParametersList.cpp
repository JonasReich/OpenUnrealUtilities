// Copyright (c) 2023 Jonas Reich & Contributors

#include "MaterialAnalyzer/OUUMaterialAnalyzer_ParametersList.h"

#include "Brushes/SlateColorBrush.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer_EditorObject.h"
#include "OUUMaterialEditingLibrary.h"
#include "Widgets/Input/SButton.h"

namespace OUU::Editor::Private::MaterialAnalyzer::Widgets
{
	void SOUUMaterialAnalyzer_ParametersListItem::Construct(
		const FArguments& InArgs,
		const TSharedRef<STableViewBase>& OwnerTableView)
	{
		ParameterData = InArgs._ParameterData;
		ColumnSizeData = InArgs._ColumnSizeData;

		// clang-format off
		this->ChildSlot[
			SNew(SBorder)
			.Padding(0.0f)
			.BorderImage(this, &SOUUMaterialAnalyzer_ParametersListItem::GetParameterListBorderImage)
			[
				Widgets::Splitter(ColumnSizeData,
					// left
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.FillWidth(1.f)
					.Padding(3.f, 0.f)
					[
						SNew(STextBlock)
						.Text(FText::FromName(ParameterData->Info.Name))
					]
					+SHorizontalBox::Slot()
					.AutoWidth()
					.Padding(3.f, 0.f)
					[
						SNew(SButton)
						.Text(INVTEXT("Open"))
						.OnClicked(this, &SOUUMaterialAnalyzer_ParametersListItem::OnParameterButtonClicked)
					],
					// right
					SNew(STextBlock)
					.Text(ParameterData->Source == FOUUMaterialAnalyzer_ParameterData::ESource::MaterialFunction ? FText::FromString(ParameterData->Expression->GetOuter()->GetName()) : FText::GetEmpty())
				)
			]
		];
		// clang-format on

		STableRow<TSharedPtr<FOUUMaterialAnalyzer_ParameterData>>::ConstructInternal(
			STableRow::FArguments().Style(FAppStyle::Get(), "DetailsView.TreeView.TableRow").ShowSelection(false),
			OwnerTableView);
	}

	const FSlateBrush* SOUUMaterialAnalyzer_ParametersListItem::GetParameterListBorderImage() const
	{
		return IsHovered() ? new FSlateColorBrush(FAppStyle::Get().GetSlateColor("Colors.Hover"))
						   : new FSlateColorBrush(FAppStyle::Get().GetSlateColor("Colors.Header"));
	}

	FReply SOUUMaterialAnalyzer_ParametersListItem::OnParameterButtonClicked() const
	{
		UOUUMaterialEditingLibrary::OpenMaterialEditorAndJumpToExpression(ParameterData->Expression);
		return FReply::Handled();
	}

	void SOUUMaterialAnalyzer_ParametersList::Construct(const FArguments& InArgs)
	{
		ColumnSizeData = InArgs._ColumnSizeData;

		SListView<TSharedPtr<FOUUMaterialAnalyzer_ParameterData>>::Construct(
			SListView::FArguments()
				.ListItemsSource(&GetMutableDefault<UOUUMaterialAnalyzer_EditorObject>()->Parameters)
				.SelectionMode(ESelectionMode::None)
				.OnGenerateRow(this, &SOUUMaterialAnalyzer_ParametersList::GenerateRowWidget)
				.ExternalScrollbar(InArgs._Scrollbar));
	}

	TSharedRef<ITableRow> SOUUMaterialAnalyzer_ParametersList::GenerateRowWidget(
		// ReSharper disable once CppPassValueParameterByConstReference
		TSharedPtr<FOUUMaterialAnalyzer_ParameterData> Item,
		const TSharedRef<STableViewBase>& OwnerTable) const
	{
		return SNew(SOUUMaterialAnalyzer_ParametersListItem, OwnerTable)
			.ParameterData(Item)
			.ColumnSizeData(ColumnSizeData);
	}
} // namespace OUU::Editor::Private::MaterialAnalyzer::Widgets
