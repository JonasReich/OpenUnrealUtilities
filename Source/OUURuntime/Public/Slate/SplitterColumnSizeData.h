// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Misc/Attribute.h"
#include "Widgets/Layout/SSplitter.h"

struct OUURUNTIME_API FSplitterColumnSizeData : public TSharedFromThis<FSplitterColumnSizeData>
{
	FSplitterColumnSizeData();
	explicit FSplitterColumnSizeData(float InColumnWidth);

	TAttribute<float> LeftColumnWidth;
	TAttribute<float> RightColumnWidth;
	SSplitter::FOnSlotResized OnWidthChanged;

	TSharedRef<SWidget> MakeSimpleSplitter(
		const TSharedRef<SWidget>& LeftWidget,
		const TSharedRef<SWidget>& RightWidget,
		const EOrientation Orientation = EOrientation::Orient_Horizontal) const;
	TSharedRef<SWidget> MakeSimpleDetailsSplitter(
		const FText& Label,
		const FText& Tooltip,
		const TSharedRef<SWidget>& RightWidget,
		const EOrientation Orientation = EOrientation::Orient_Horizontal) const;

private:
	float ColumnWidth = 0.3f;
	float OnGetLeftColumnWidth() const { return 1.0f - ColumnWidth; }
	float OnGetRightColumnWidth() const { return ColumnWidth; }
	void OnSetColumnWidth(float InWidth) { ColumnWidth = InWidth; }
};

struct OUURUNTIME_API FSplitterMultiSizeData : public TSharedFromThis<FSplitterMultiSizeData>
{
	TSharedRef<SWidget> MakeSimpleSplitter(
		const TArray<TSharedPtr<SWidget>>& Widgets,
		TArray<float> InDefaultSizes = {},
		const EOrientation Orientation = EOrientation::Orient_Horizontal);

private:
	TArray<float> EntrySizes;
	float OnGetEntrySize(const int32 InIndex) const { return InIndex < EntrySizes.Num() ? EntrySizes[InIndex] : 0.0f; }
	void OnSetEntrySize(const int32 InIndex, float InSize);
};
