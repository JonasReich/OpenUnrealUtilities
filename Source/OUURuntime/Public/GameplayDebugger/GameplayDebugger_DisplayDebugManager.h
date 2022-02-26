
#pragma once

#include "CoreMinimal.h"

#include "CanvasItem.h"
#include "Engine/Canvas.h"

struct OUURUNTIME_API FGameplayDebugger_DisplayDebugManager
{
private:
	FCanvasTextItem DebugTextItem;
	FVector2D CurrentPos;
	float NextColumXPos;
	float MaxCharHeight;
	FVector2D InitialPos;
	class UCanvas* Canvas;

public:
	FGameplayDebugger_DisplayDebugManager() :
		DebugTextItem(FCanvasTextItem(FVector2D(0, 0), FText::GetEmpty(), nullptr, FLinearColor::White)),
		CurrentPos(FVector2D::ZeroVector),
		NextColumXPos(0.f),
		MaxCharHeight(0.f),
		InitialPos(FVector2D::ZeroVector),
		Canvas(nullptr)
	{
		DebugTextItem.EnableShadow(FLinearColor::Black);
	}

	void Initialize(class UCanvas* InCanvas, const UFont* NewFont, FVector2D InInitialPosition)
	{
		SetFont(NewFont);
		Canvas = InCanvas;
		InitialPos = InInitialPosition;
		CurrentPos = InitialPos;
		NextColumXPos = 0.f;
	}

	void SetFont(const UFont* NewFont)
	{
		if (NewFont && (NewFont != DebugTextItem.Font))
		{
			DebugTextItem.Font = NewFont;
			MaxCharHeight = DebugTextItem.Font->GetMaxCharHeight();
		}
	}

	void SetDrawColor(const FColor& NewColor) { DebugTextItem.SetColor(NewColor.ReinterpretAsLinear()); }

	void SetLinearDrawColor(const FLinearColor& NewColor) { DebugTextItem.SetColor(NewColor); }

	void DrawString(const FString& InDebugString, const float& OptionalXOffset = 0.f);

	void AddColumnIfNeeded();

	float GetTextScale() const;

	float GetYStep() const { return MaxCharHeight * 1.15f * GetTextScale(); }

	float GetXPos() const { return CurrentPos.X; }

	float GetYPos() const { return CurrentPos.Y; }

	float& GetYPosRef() { return CurrentPos.Y; }

	void SetYPos(const float NewYPos) { CurrentPos.Y = NewYPos; }

	float GetMaxCharHeight() const { return MaxCharHeight; }

	float& GetMaxCharHeightRef() { return MaxCharHeight; }

	void ShiftYDrawPosition(const float& YOffset)
	{
		CurrentPos.Y += YOffset;
		AddColumnIfNeeded();
	}
};

inline void FGameplayDebugger_DisplayDebugManager::DrawString(
	const FString& InDebugString,
	const float& OptionalXOffset)
{
	if (Canvas)
	{
		const float TextScale = GetTextScale();
		DebugTextItem.Scale = FVector2D(TextScale, TextScale);

		DebugTextItem.Text = FText::FromString(InDebugString);
		Canvas->DrawItem(DebugTextItem, FVector2D(CurrentPos.X + OptionalXOffset, CurrentPos.Y));

		NextColumXPos = FMath::Max(NextColumXPos, CurrentPos.X + OptionalXOffset + DebugTextItem.DrawnSize.X);
		CurrentPos.Y += FMath::Max(GetYStep(), DebugTextItem.DrawnSize.Y);
		AddColumnIfNeeded();
	}
}

inline float FGameplayDebugger_DisplayDebugManager::GetTextScale() const
{
	return Canvas ? FMath::Max(Canvas->SizeX / 1920.f, 1.f) : 1.f;
}

inline void FGameplayDebugger_DisplayDebugManager::AddColumnIfNeeded()
{
	if (Canvas)
	{
		const float YStep = GetYStep();
		if ((CurrentPos.Y + YStep) > Canvas->SizeY)
		{
			CurrentPos.Y = InitialPos.Y;
			CurrentPos.X = NextColumXPos + YStep * 2.f;
		}
	}
}
