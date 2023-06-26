// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "OUUMaterialAnalyzer_ParameterData.h"
#include "Slate/SplitterColumnSizeData.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"

namespace OUU::Editor::Private::MaterialAnalyzer::Widgets
{
	class SOUUMaterialAnalyzer_ParametersListItem : public STableRow<TSharedPtr<FOUUMaterialAnalyzer_ParameterData>>
	{
	public:
		SLATE_BEGIN_ARGS(SOUUMaterialAnalyzer_ParametersListItem)
		: _ParameterData(nullptr),
		_ColumnSizeData(nullptr)
	{
	}
		SLATE_ARGUMENT(TSharedPtr<FOUUMaterialAnalyzer_ParameterData>, ParameterData)
		SLATE_ARGUMENT(TSharedPtr<FSplitterColumnSizeData>, ColumnSizeData)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& OwnerTableView);

	private:
		const FSlateBrush* GetBorderImage() const;

		FReply OnParameterButtonClicked() const;

	private:
		/** The node info to build the tree view row from. */
		TSharedPtr<FOUUMaterialAnalyzer_ParameterData> ParameterData;

		TSharedPtr<FSplitterColumnSizeData> ColumnSizeData = nullptr;
	};

	class SOUUMaterialAnalyzer_ParametersList : public SListView<TSharedPtr<FOUUMaterialAnalyzer_ParameterData>>
	{
	public:
		SLATE_BEGIN_ARGS(SOUUMaterialAnalyzer_ParametersList)
		:_Scrollbar(nullptr),
		_ColumnSizeData(nullptr)
	{
	}
		SLATE_ARGUMENT(TSharedPtr<SScrollBar>, Scrollbar)
		SLATE_ARGUMENT(TSharedPtr<FSplitterColumnSizeData>, ColumnSizeData)
		SLATE_END_ARGS()

		void Construct(const FArguments& InArgs);

		bool HasAnyItems() const { return GetItems().Num() > 0; }

	private:
		TSharedRef<ITableRow> GenerateRowWidget(
			TSharedPtr<FOUUMaterialAnalyzer_ParameterData> Item,
			const TSharedRef<STableViewBase>& OwnerTable) const;

		TSharedPtr<FSplitterColumnSizeData> ColumnSizeData = nullptr;
	};
} // namespace OUU::Editor::Private::MaterialAnalyzer::Widgets
