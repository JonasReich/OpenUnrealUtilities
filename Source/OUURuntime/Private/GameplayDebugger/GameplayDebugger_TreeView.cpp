// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayDebugger/GameplayDebugger_TreeView.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "DrawDebugHelpers.h"

	#include "Engine/Canvas.h"
	#include "GameplayDebuggerTypes.h"

constexpr int32 NumberOfColumnsPerScreen = 2;

FGameplayDebugger_TreeView::FGameplayDebugger_TreeView(
	FGameplayDebuggerCanvasContext& InCanvasContext,
	FColor InDrawColor) :
	DrawColor(InDrawColor), CanvasContext(InCanvasContext)
{
	MaxCursorY = FMath::Max(CanvasContext.CursorY, MaxCursorY);
}

void FGameplayDebugger_TreeView::DrawString(const FString& InDebugString, const float& OptionalXOffset)
{
	AddColumnIfNeeded();
	const float PreviousY = CanvasContext.CursorY;
	{
		TGuardValue<float> ScopedCursorX(CanvasContext.CursorX, CanvasContext.CursorX + OptionalXOffset);
		CanvasContext.Print(DrawColor, InDebugString);
	}
	float SizeX = 0.0f, SizeY = 0.0f;
	CanvasContext.MeasureString(InDebugString, OUT SizeX, OUT SizeY);
	NextColumnXPos = FMath::Max(NextColumnXPos, CanvasContext.CursorX + OptionalXOffset + SizeX);
	CanvasContext.CursorY = FMath::Max(PreviousY + GetLineHeight(), CanvasContext.CursorY);
	MaxCursorY = FMath::Max(CanvasContext.CursorY, MaxCursorY);
	AddColumnIfNeeded();
}

void FGameplayDebugger_TreeView::AddColumnIfNeeded()
{
	const float LineHeight = GetLineHeight();
	if (CanvasContext.Canvas.IsValid() && (CanvasContext.CursorY + LineHeight * 2) > CanvasContext.Canvas->ClipY)
	{
		CanvasContext.DefaultX += CanvasContext.Canvas->ClipX / NumberOfColumnsPerScreen;
		CanvasContext.CursorX = CanvasContext.DefaultX;
		CanvasContext.CursorY = CanvasContext.DefaultY;

		for (auto& Entry : StartingPointsForIndentLevels)
		{
			Entry.Y = CanvasContext.DefaultY;
		}
	}
}

float FGameplayDebugger_TreeView::GetLineHeight() const
{
	return CanvasContext.Font->GetMaxCharHeight() * 1.15f;
}

void FGameplayDebugger_TreeView::DrawTree_Impl(TArray<FFlattenedDebugData> FlattenedDebugData)
{
	auto* Canvas = CanvasContext.Canvas.Get();
	if (!IsValid(Canvas))
		return;

	constexpr float NodeIndent = 8.f;
	constexpr float LineIndent = 4.f;
	constexpr float AttachLineLength = NodeIndent - LineIndent;
	int32 PrevChainID = -1;

	for (auto& DebugItem : FlattenedDebugData)
	{
		float CurrIndent = DebugItem.Indent * NodeIndent;
		float CurrentLineBase_Y = CanvasContext.CursorY + CanvasContext.Font->GetMaxCharHeight();

		if (PrevChainID != DebugItem.ChainID)
		{
			const int32 HalfStep = static_cast<int32>(CanvasContext.Font->GetMaxCharHeight() / 2);
			CanvasContext.CursorY += HalfStep;
			MaxCursorY = FMath::Max(CanvasContext.CursorY, MaxCursorY);
			AddColumnIfNeeded();

			const int32 VerticalLineIndex = DebugItem.Indent - 1;
			if (StartingPointsForIndentLevels.IsValidIndex(VerticalLineIndex))
			{
				const FVector2D LineStartCoord = StartingPointsForIndentLevels[VerticalLineIndex];

				StartingPointsForIndentLevels[VerticalLineIndex] = FVector2D(CanvasContext.CursorX, CurrentLineBase_Y);

				// Only draw line, if indent parent is in same column as current one
				if (FMath::IsNearlyEqual(LineStartCoord.X, CanvasContext.CursorX))
				{
					const float EndX = CanvasContext.CursorX + CurrIndent;
					const float StartX = EndX - AttachLineLength;
					DrawDebugCanvas2DLine(
						Canvas,
						FVector(StartX, CurrentLineBase_Y, 0.f),
						FVector(EndX, CurrentLineBase_Y, 0.f),
						FColor::White);
					DrawDebugCanvas2DLine(
						Canvas,
						FVector(StartX, LineStartCoord.Y, 0.f),
						FVector(StartX, CurrentLineBase_Y, 0.f),
						FColor::White);
				}
			}

			// move CurrentLineBase_Y back to base of line
			CurrentLineBase_Y += HalfStep;
		}

		if (!StartingPointsForIndentLevels.IsValidIndex(DebugItem.Indent))
		{
			StartingPointsForIndentLevels.AddZeroed(DebugItem.Indent + 1 - StartingPointsForIndentLevels.Num());
		}
		StartingPointsForIndentLevels[DebugItem.Indent] = FVector2D(CanvasContext.CursorX, CurrentLineBase_Y);

		PrevChainID = DebugItem.ChainID;
		DrawString(DebugItem.DebugString, CurrIndent);
	}
}

#endif
