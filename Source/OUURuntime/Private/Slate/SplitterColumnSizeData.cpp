// Copyright (c) 2022 Jonas Reich

#include "Slate/SplitterColumnSizeData.h"

#include "Widgets/Text/STextBlock.h"

FSplitterColumnSizeData::FSplitterColumnSizeData() : FSplitterColumnSizeData(0.3f) {}

FSplitterColumnSizeData::FSplitterColumnSizeData(float InColumnWidth) : ColumnWidth(InColumnWidth)
{
	using FGetter = TAttribute<float>::FGetter;

	LeftColumnWidth =
		TAttribute<float>::Create(FGetter::CreateRaw(this, &FSplitterColumnSizeData::OnGetLeftColumnWidth));
	RightColumnWidth =
		TAttribute<float>::Create(FGetter::CreateRaw(this, &FSplitterColumnSizeData::OnGetRightColumnWidth));
	OnWidthChanged = SSplitter::FOnSlotResized::CreateRaw(this, &FSplitterColumnSizeData::OnSetColumnWidth);
}

TSharedRef<SWidget> FSplitterColumnSizeData::MakeSimpleDetailsSplitter(
	const FText& Label,
	const FText& Tooltip,
	TSharedRef<SWidget> RightWidget) const
{
	// clang-format off
	return SNew(SSplitter)
		+ SSplitter::Slot()
		.SizeRule(SSplitter::ESizeRule::FractionOfParent)
		.Value(LeftColumnWidth)
		.OnSlotResized(OnWidthChanged)
		[
			SNew(STextBlock)
				.Text(Label)
				.ToolTipText(Tooltip)
		]
		+ SSplitter::Slot()
		.SizeRule(SSplitter::ESizeRule::FractionOfParent)
		.Value(RightColumnWidth)
		.OnSlotResized(OnWidthChanged)
		[
			RightWidget
		];
	// clang-format on
}
