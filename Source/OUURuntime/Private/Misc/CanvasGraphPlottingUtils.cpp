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
	const FString GraphTitle,
	float HighestValue,
	bool bUseLogarithmicYAxis)
{
	const int32 NumStats = StatsToDraw.Num();
	if (NumStats <= 0)
		return;
	const int32 NumberOfSamples = StatsToDraw[0].ValueAggregator.Num();

	const UFont* SmallFont = GEngine->GetSmallFont();
	const float FontHeight = SmallFont->GetStringHeightSize(TEXT("000.0"));

	// Graph layout

	const float GraphBackgroundMarginSize = 8.0f;
	const float GraphTotalWidth = 200.f;
	const float GraphHorizPixelsPerFrame = GraphTotalWidth / NumberOfSamples;
	const float GraphTotalHeight = 200.f;

	// Draw background.
	{
		FLinearColor BackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.7f);
		FCanvasTileItem BackgroundTile(
			FVector2D(
				GraphLeftXPos - GraphBackgroundMarginSize,
				GraphBottomYPos - GraphTotalHeight - GraphBackgroundMarginSize),
			FVector2D(
				GraphTotalWidth + 2 * GraphBackgroundMarginSize,
				GraphTotalHeight + 2 * GraphBackgroundMarginSize),
			BackgroundColor);

		BackgroundTile.BlendMode = SE_BLEND_AlphaBlend;

		InCanvas->DrawItem(BackgroundTile);
	}

	FBatchedElements* BatchedElements = InCanvas->GetBatchedElements(FCanvas::ET_Line);
	FHitProxyId HitProxyId = InCanvas->GetHitProxyId();

	// Reserve line vertices (2 border lines, then up to the maximum number of graph lines)
	BatchedElements->AddReserveLines(2 + NumStats * NumberOfSamples);

	// Draw timing graph frame.
	{
		const FLinearColor GraphBorderColor(0.1f, 0.1f, 0.1f);

		// Left
		BatchedElements->AddLine(
			FVector(GraphLeftXPos - 1.0f, GraphBottomYPos - GraphTotalHeight - GraphBackgroundMarginSize, 0.0f),
			FVector(GraphLeftXPos - 1.0f, GraphBottomYPos - 1.0f, 0.0f),
			GraphBorderColor,
			HitProxyId);

		// Bottom
		BatchedElements->AddLine(
			FVector(GraphLeftXPos - 1.0f, GraphBottomYPos - 1.0f, 0.0f),
			FVector(
				GraphLeftXPos + GraphHorizPixelsPerFrame * NumberOfSamples + GraphBackgroundMarginSize,
				GraphBottomYPos - 1.0f,
				0.0f),
			GraphBorderColor,
			HitProxyId);

		InCanvas->DrawShadowedString(
			GraphLeftXPos - GraphBackgroundMarginSize,
			GraphBottomYPos - GraphTotalHeight - GraphBackgroundMarginSize - FontHeight - 2.0f,
			*FString::Printf(TEXT("%s (%.2f)"), *GraphTitle, HighestValue),
			SmallFont,
			GraphBorderColor);
	}

	for (auto& Stat : StatsToDraw)
	{
		// For each sample in our data set
		for (int32 CurFrameIndex = 0; CurFrameIndex < NumberOfSamples; ++CurFrameIndex)
		{
			const int32 PrevFrameIndex = FMath::Max(0, CurFrameIndex - 1);

			const float PrevValue = Stat.ValueAggregator[PrevFrameIndex];
			const float CurValue = Stat.ValueAggregator[CurFrameIndex];
			float PrevValueNormalized = bUseLogarithmicYAxis
				? UOUUMathLibrary::LinearValueToNormalizedLogScale(PrevValue, 1.f, HighestValue)
				: PrevValue / HighestValue;
			float CurValueNormalized = bUseLogarithmicYAxis
				? UOUUMathLibrary::LinearValueToNormalizedLogScale(CurValue, 1.f, HighestValue)
				: CurValue / HighestValue;

			const float PrevValuePixels = PrevValueNormalized * GraphTotalHeight;
			const float CurValuePixels = CurValueNormalized * GraphTotalHeight;

			if (CurValuePixels < 0.0f || PrevValuePixels < 0.0f)
			{
				continue;
			}

			const FVector LineStart(
				GraphLeftXPos + StaticCast<float>(PrevFrameIndex) * GraphHorizPixelsPerFrame,
				GraphBottomYPos - FMath::Min(PrevValuePixels, GraphTotalHeight),
				0.0f);

			const FVector LineEnd(
				GraphLeftXPos + StaticCast<float>(CurFrameIndex) * GraphHorizPixelsPerFrame,
				GraphBottomYPos - FMath::Min(CurValuePixels, GraphTotalHeight),
				0.0f);

			BatchedElements->AddLine(LineStart, LineEnd, Stat.Color, HitProxyId);

			if (CurFrameIndex == NumberOfSamples - 1)
			{
				InCanvas->DrawShadowedString(
					GraphLeftXPos + GraphTotalWidth + GraphBackgroundMarginSize * 2.f + 4.0f,
					LineEnd.Y - FontHeight / 2,
					*FString::Printf(TEXT("%s (%.2f)"), *Stat.Title, CurValue),
					SmallFont,
					Stat.Color);
			}
		}
	}
}
