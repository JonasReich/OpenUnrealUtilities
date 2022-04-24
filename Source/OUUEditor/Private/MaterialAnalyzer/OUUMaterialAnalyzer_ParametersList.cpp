// Copyright (c) 2022 Jonas Reich

#include "MaterialAnalyzer/OUUMaterialAnalyzer_ParametersList.h"

#include "IMaterialEditor.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "MaterialAnalyzer/OUUMaterialAnalyzer_EditorObject.h"
#include "MaterialEditorUtilities.h"
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
			.BorderImage(this, &SOUUMaterialAnalyzer_ParametersListItem::GetBorderImage)
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
			STableRow::FArguments().Style(FEditorStyle::Get(), "DetailsView.TreeView.TableRow").ShowSelection(false),
			OwnerTableView);
	}

	const FSlateBrush* SOUUMaterialAnalyzer_ParametersListItem::GetBorderImage() const
	{
		return IsHovered() ? FEditorStyle::GetBrush("DetailsView.CategoryMiddle_Hovered")
						   : FEditorStyle::GetBrush("DetailsView.CategoryMiddle");
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
		TSharedPtr<FOUUMaterialAnalyzer_ParameterData> Item,
		const TSharedRef<STableViewBase>& OwnerTable) const
	{
		return SNew(SOUUMaterialAnalyzer_ParametersListItem, OwnerTable)
			.ParameterData(Item)
			.ColumnSizeData(ColumnSizeData);
	}
} // namespace OUU::Editor::Private::MaterialAnalyzer::Widgets
