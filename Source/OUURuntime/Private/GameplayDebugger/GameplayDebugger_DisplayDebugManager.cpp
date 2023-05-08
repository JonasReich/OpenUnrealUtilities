// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayDebugger/GameplayDebugger_DisplayDebugManager.h"

#include "DisplayDebugHelpers.h"
#include "DrawDebugHelpers.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "Engine/Canvas.h"
	#include "GameplayDebuggerTypes.h"

constexpr int32 NumberOfColumnsPerScreen = 2;

FGameplayDebugger_DisplayDebugManager::FGameplayDebugger_DisplayDebugManager(
	FGameplayDebuggerCanvasContext& InCanvasContext) :
	CanvasContext(InCanvasContext)
{
	NextColumnXPos = 0.f;
	MaxCursorY = FMath::Max(CanvasContext.CursorY, MaxCursorY);
}

void FGameplayDebugger_DisplayDebugManager::SetDrawColor(const FColor& NewColor)
{
	DrawColor = NewColor;
}

void FGameplayDebugger_DisplayDebugManager::SetLinearDrawColor(const FLinearColor& NewColor)
{
	DrawColor = NewColor.ToFColor(true);
}

void FGameplayDebugger_DisplayDebugManager::DrawString(const FString& InDebugString, const float& OptionalXOffset)
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
	CanvasContext.CursorY = FMath::Max(PreviousY + GetYStep(), CanvasContext.CursorY);
	MaxCursorY = FMath::Max(CanvasContext.CursorY, MaxCursorY);
	AddColumnIfNeeded();
}

void FGameplayDebugger_DisplayDebugManager::AddColumnIfNeeded()
{
	const float YStep = GetYStep();
	if (CanvasContext.Canvas.IsValid() && (CanvasContext.CursorY + YStep * 2) > CanvasContext.Canvas->ClipY)
	{
		CanvasContext.DefaultX += CanvasContext.Canvas->ClipX / NumberOfColumnsPerScreen;
		CanvasContext.CursorX = CanvasContext.DefaultX;
		CanvasContext.CursorY = CanvasContext.DefaultY;
	}
}

float FGameplayDebugger_DisplayDebugManager::GetYStep() const
{
	return CanvasContext.Font->GetMaxCharHeight() * 1.15f;
}

float FGameplayDebugger_DisplayDebugManager::GetXPos() const
{
	return CanvasContext.CursorX;
}

float FGameplayDebugger_DisplayDebugManager::GetYPos() const
{
	return CanvasContext.CursorY;
}

void FGameplayDebugger_DisplayDebugManager::SetYPos(const float NewYPos)
{
	CanvasContext.CursorY = NewYPos;
	MaxCursorY = FMath::Max(CanvasContext.CursorY, MaxCursorY);
}

float FGameplayDebugger_DisplayDebugManager::GetMaxCharHeight() const
{
	return CanvasContext.Font->GetMaxCharHeight();
}

void FGameplayDebugger_DisplayDebugManager::ShiftYDrawPosition(const float& YOffset)
{
	CanvasContext.CursorY += YOffset;
	MaxCursorY = FMath::Max(CanvasContext.CursorY, MaxCursorY);
	AddColumnIfNeeded();
}

void FGameplayDebugger_DisplayDebugManager::DrawTree_Impl(TArray<FFlattenedDebugData> LineHelpers, float& Indent)
{
	auto* Canvas = CanvasContext.Canvas.Get();
	if (!IsValid(Canvas))
		return;

	constexpr float NodeIndent = 8.f;
	constexpr float LineIndent = 4.f;
	constexpr float AttachLineLength = NodeIndent - LineIndent;
	int32 PrevChainID = -1;
	SetLinearDrawColor(FColor::White);

	FIndenter AnimNodeTreeIndent(Indent);
	// Index represents indent level, track the current starting point for that
	TArray<FVector2D> IndentLineStartCoord;
	for (auto& Line : LineHelpers)
	{
		float CurrIndent = Indent + (Line.Indent * NodeIndent);
		float CurrLineYBase = GetYPos() + GetMaxCharHeight();

		if (PrevChainID != Line.ChainID)
		{
			// Extra spacing to delimit different chains, CurrLineYBase now
			// roughly represents middle of text line, so we can use it for line drawing
			const int32 HalfStep = static_cast<int32>(GetMaxCharHeight() / 2);
			ShiftYDrawPosition(static_cast<float>(HalfStep));

			// Handle line drawing
			const int32 VerticalLineIndex = Line.Indent - 1;
			if (IndentLineStartCoord.IsValidIndex(VerticalLineIndex))
			{
				const FVector2D LineStartCoord = IndentLineStartCoord[VerticalLineIndex];
				IndentLineStartCoord[VerticalLineIndex] = FVector2D(GetXPos(), CurrLineYBase);

				// If indent parent is not in same column, ignore line.
				if (FMath::IsNearlyEqual(LineStartCoord.X, GetXPos()))
				{
					const float EndX = GetXPos() + CurrIndent;
					const float StartX = EndX - AttachLineLength;

					// horizontal line to node
					DrawDebugCanvas2DLine(
						Canvas,
						FVector(StartX, CurrLineYBase, 0.f),
						FVector(EndX, CurrLineYBase, 0.f),
						FColor::White);

					// vertical line
					DrawDebugCanvas2DLine(
						Canvas,
						FVector(StartX, LineStartCoord.Y, 0.f),
						FVector(StartX, CurrLineYBase, 0.f),
						FColor::White);
				}
			}

			// move CurrYLineBase back to base of line
			CurrLineYBase += HalfStep;
		}

		// Update our base position for subsequent line drawing
		if (!IndentLineStartCoord.IsValidIndex(Line.Indent))
		{
			IndentLineStartCoord.AddZeroed(Line.Indent + 1 - IndentLineStartCoord.Num());
		}
		IndentLineStartCoord[Line.Indent] = FVector2D(GetXPos(), CurrLineYBase);

		PrevChainID = Line.ChainID;
		DrawString(Line.DebugString, CurrIndent);
	}
}

#endif
