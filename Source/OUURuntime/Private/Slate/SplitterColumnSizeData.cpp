// Copyright (c) 2022 Jonas Reich

#include "Slate/SplitterColumnSizeData.h"

FSplitterColumnSizeData::FSplitterColumnSizeData()
{
	using FGetter = TAttribute<float>::FGetter;

	LeftColumnWidth =
		TAttribute<float>::Create(FGetter::CreateRaw(this, &FSplitterColumnSizeData::OnGetLeftColumnWidth));
	RightColumnWidth =
		TAttribute<float>::Create(FGetter::CreateRaw(this, &FSplitterColumnSizeData::OnGetRightColumnWidth));
	OnWidthChanged = SSplitter::FOnSlotResized::CreateRaw(this, &FSplitterColumnSizeData::OnSetColumnWidth);
}
