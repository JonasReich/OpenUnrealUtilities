// Copyright (c) 2023 Jonas Reich & Contributors

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

TSharedRef<SWidget> FSplitterColumnSizeData::MakeSimpleSplitter(
	TSharedRef<SWidget> LeftWidget,
	TSharedRef<SWidget> RightWidget,
	const EOrientation Orientation) const
{
	// clang-format off
	return SNew(SSplitter)
		.Orientation(Orientation)
		+ SSplitter::Slot()
		.SizeRule(SSplitter::ESizeRule::FractionOfParent)
		.Value(LeftColumnWidth)
		.OnSlotResized(OnWidthChanged)
		[
			LeftWidget
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

TSharedRef<SWidget> FSplitterColumnSizeData::MakeSimpleDetailsSplitter(
	const FText& Label,
	const FText& Tooltip,
	TSharedRef<SWidget> RightWidget,
	const EOrientation Orientation) const
{
	// clang-format off
	return SNew(SSplitter)
		.Orientation(Orientation)
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

TSharedRef<SWidget> FSplitterMultiSizeData::MakeSimpleSplitter(
	const TArray<TSharedPtr<SWidget>>& Widgets,
	TArray<float> DefaultColumnSizes,
	const EOrientation Orientation)
{
	if (Widgets.IsEmpty())
	{
		return SNullWidget::NullWidget;
	}

	if (Widgets.Num() == 1)
	{
		return Widgets[0].ToSharedRef();
	}

	EntrySizes = MoveTemp(DefaultColumnSizes);
	if (EntrySizes.Num() < Widgets.Num())
	{
		const int32 PrevNum = EntrySizes.Num();
		EntrySizes.SetNum(Widgets.Num());
		for (int32 i = PrevNum; i < EntrySizes.Num(); ++i)
		{
			EntrySizes[i] = 1.0f / EntrySizes.Num();
		}
	}

	// Normalize entry sizes
	float TotalSizes = 0.0f;
	for (const auto Size : EntrySizes)
	{
		TotalSizes += Size;
	}

	if (TotalSizes > 0.0f)
	{
		for (auto& Size : EntrySizes)
		{
			Size /= TotalSizes;
		}
	}

	auto Splitter = SNew(SSplitter).Orientation(Orientation);
	for (int32 i = 0; i < Widgets.Num(); ++i)
	{
		Splitter->AddSlot()
			.SizeRule(SSplitter::ESizeRule::FractionOfParent)
			.Value_Lambda([this, i]() { return OnGetEntrySize(i); })
			.OnSlotResized_Lambda([this, i](float InSize) { OnSetEntrySize(i, InSize); })[Widgets[i].ToSharedRef()];
	}

	return Splitter;
}

void FSplitterMultiSizeData::OnSetEntrySize(const int32 InIndex, float InSize)
{
	if (InIndex >= EntrySizes.Num())
	{
		return;
	}

	const float Delta = InSize - EntrySizes[InIndex];
	EntrySizes[InIndex] = InSize;

	if (EntrySizes.Num() > 1)
	{
		int32 ResizeIndex = InIndex + 1;
		if (ResizeIndex == EntrySizes.Num())
		{
			ResizeIndex = InIndex - 1;
		}

		EntrySizes[ResizeIndex] -= Delta;
	}
}