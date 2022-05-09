﻿// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Misc/Attribute.h"
#include "Widgets/Layout/SSplitter.h"

struct OUURUNTIME_API FSplitterColumnSizeData : public TSharedFromThis<FSplitterColumnSizeData>
{
	FSplitterColumnSizeData();

	TAttribute<float> LeftColumnWidth;
	TAttribute<float> RightColumnWidth;
	SSplitter::FOnSlotResized OnWidthChanged;

private:
	float ColumnWidth = 0.5f;
	float OnGetLeftColumnWidth() const { return 1.0f - ColumnWidth; }
	float OnGetRightColumnWidth() const { return ColumnWidth; }
	void OnSetColumnWidth(float InWidth) { ColumnWidth = InWidth; }
};