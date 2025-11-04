// Copyright (c) 2023 Jonas Reich & Contributors

#include "Misc/CanvasGraphPlottingUtils.h"

#include "BatchedElements.h"
#include "CanvasItem.h"
#include "CanvasTypes.h"
#include "Engine/Engine.h"
#include "HitProxies.h"
#include "Math/OUUMathLibrary.h"

void OUU::Runtime::CanvasGraphPlottingUtils::DrawCanvasGraph(
	FCanvas* InCanvas,
	float GraphLeftXPos,
	float GraphBottomYPos,
	const TArray<OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData>& StatsToDraw,
	const FString& GraphTitle,
	float HighestValue,
	bool bUseLogarithmicYAxis,
	TArray<FLimit> Limits)
{
	const int32 NumStats = StatsToDraw.Num();
	if (NumStats <= 0)
		return;

	// Assume all stats have the same number of samples, or at least the same pixel density.
	// So the first series is used for determining density to fit it entirely on the plot.
	const int32 NumberOfSamples = StatsToDraw[0].ValueAggregator.Num();

	// Make sure the vertical value range expands encompass to at least the limits.
	// The vertical coordinate calculations below assume a base line of 0 and no negative numbers to be plotted.
	for (auto& Limit : Limits)
	{
		HighestValue = FMath::Max(HighestValue, Limit.Value);
	}

	const UFont* SmallFont = GEngine->GetSmallFont();
	const float FontHeight = SmallFont->GetStringHeightSize(TEXT("000.0"));

	// Graph layout

	constexpr float GraphBackgroundMarginSize = 8.0f;
	constexpr float GraphTotalWidth = 200.f;
	constexpr float GraphTotalHeight = 200.f;

	// Draw background.
	{
		FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);
		FCanvasTileItem BackgroundTile(
			FVector2D(
				GraphLeftXPos - GraphBackgroundMarginSize,
				GraphBottomYPos - GraphTotalHeight - GraphBackgroundMarginSize),
			FVector2D(
				GraphTotalWidth + 2.0f * GraphBackgroundMarginSize,
				GraphTotalHeight + 2.0f * GraphBackgroundMarginSize),
			BackgroundColor);

		BackgroundTile.BlendMode = SE_BLEND_AlphaBlend;

		InCanvas->DrawItem(BackgroundTile);
	}

	FBatchedElements* BatchedElements = InCanvas->GetBatchedElements(FCanvas::ET_Line);
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();

	// Reserve line vertices (2 border lines, then up to the maximum number of graph lines + limits)
	BatchedElements->AddReserveLines(2 + NumStats * NumberOfSamples + Limits.Num());

	// Draw timing graph frame.
	{
		constexpr FLinearColor GraphBorderColor(0.1f, 0.1f, 0.1f);

		// Left
		BatchedElements->AddLine(
			FVector(GraphLeftXPos - 1.0f, GraphBottomYPos - GraphTotalHeight - GraphBackgroundMarginSize, 0.0f),
			FVector(GraphLeftXPos - 1.0f, GraphBottomYPos - 1.0f, 0.0f),
			GraphBorderColor,
			HitProxyId);

		// Bottom
		BatchedElements->AddLine(
			FVector(GraphLeftXPos - 1.0f, GraphBottomYPos - 1.0f, 0.0f),
			FVector(GraphLeftXPos + GraphTotalWidth + GraphBackgroundMarginSize, GraphBottomYPos - 1.0f, 0.0f),
			GraphBorderColor,
			HitProxyId);

		// Graph Title
		InCanvas->DrawShadowedString(
			GraphLeftXPos - GraphBackgroundMarginSize,
			GraphBottomYPos - GraphTotalHeight - GraphBackgroundMarginSize - FontHeight - 2.0f,
			*FString::Printf(TEXT("%s (%.2f)"), *GraphTitle, HighestValue),
			SmallFont,
			GraphBorderColor);
	}

	auto NormalizeValue = [&](float Value) {
		return bUseLogarithmicYAxis ? UOUUMathLibrary::LinearValueToNormalizedLogScale(Value, 1.f, HighestValue)
									: Value / HighestValue;
	};

	auto MakeLinePixelCoordinate = [&](float XPos_Normalized, float YPos_Normalized) -> FVector {
		// make sure the coordinates are clamped to 0..1 range, so we never draw outside the graph background
		XPos_Normalized = FMath::Clamp(XPos_Normalized, 0.0, 1.0);
		YPos_Normalized = FMath::Clamp(YPos_Normalized, 0.0, 1.0);

		return FVector(
			GraphLeftXPos + XPos_Normalized * GraphTotalWidth,
			GraphBottomYPos - YPos_Normalized * GraphTotalHeight,
			0.0f);
	};

	auto DrawLineAndLabel = [&](float ValueStart,
								float ValueEnd,
								float XStart_Normalized,
								float XEnd_Normalized,
								const FLinearColor& Color,
								const FString& Title,
								bool DrawLabel) {
		const FVector LineStart = MakeLinePixelCoordinate(XStart_Normalized, NormalizeValue(ValueStart));
		const FVector LineEnd = MakeLinePixelCoordinate(XEnd_Normalized, NormalizeValue(ValueEnd));
		BatchedElements->AddLine(LineStart, LineEnd, Color, HitProxyId);

		if (DrawLabel)
		{
			InCanvas->DrawShadowedString(
				GraphLeftXPos + GraphTotalWidth + GraphBackgroundMarginSize * 2.f + 4.0f,
				LineEnd.Y - FontHeight / 2,
				*FString::Printf(TEXT("%s (%.2f)"), *Title, ValueEnd),
				SmallFont,
				Color);
		}
	};

	for (auto& Stat : StatsToDraw)
	{
		// For each sample in our data set
		for (int32 CurFrameIndex = 0; CurFrameIndex < NumberOfSamples; ++CurFrameIndex)
		{
			const int32 PrevFrameIndex = FMath::Max(0, CurFrameIndex - 1);
			const float PrevValue = Stat.ValueAggregator[PrevFrameIndex];
			const float CurValue = Stat.ValueAggregator[CurFrameIndex];

			DrawLineAndLabel(
				PrevValue,
				CurValue,
				StaticCast<float>(PrevFrameIndex) / NumberOfSamples,
				StaticCast<float>(CurFrameIndex) / NumberOfSamples,
				Stat.Color,
				Stat.Title,
				CurFrameIndex == NumberOfSamples - 1);
		}
	}

	for (auto& Limit : Limits)
	{
		DrawLineAndLabel(Limit.Value, Limit.Value, 0.0, 1.0, Limit.Color, Limit.Title, true);
	}
}
