// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once
#include "ActorMapWindow/OUUActorMapWindow_TabSpawner.h"

namespace OUU::Developer::ActorMapWindow
{
	//------------------------------------------------------------------------
	// SActorLocationOverlay
	//------------------------------------------------------------------------

	/**
	 * The actual overlay widget that paints actor locations, names, etc.
	 * on-top of the scene capture in the background.
	 */
	class SActorLocationOverlay : public SLeafWidget
	{
		using Super = SLeafWidget;

		SLATE_BEGIN_ARGS(SActorLocationOverlay)
			{
			}

			SLATE_ATTRIBUTE(const TArray<TSharedPtr<FOUUActorMapQuery>>*, ActorQueries);
			SLATE_ATTRIBUTE(FVector, ReferencePosition);
			SLATE_ATTRIBUTE(float, MapSize);
			SLATE_ATTRIBUTE(EShowFlags, ShowFlags);
		SLATE_END_ARGS()

		TAttribute<const TArray<TSharedPtr<FOUUActorMapQuery>>*> ActorQueries;
		TAttribute<FVector> ReferencePosition = FVector::ZeroVector;
		TAttribute<float> MapSize = 0.f;
		TAttribute<EShowFlags> ShowFlags;
		
#if WITH_EDITOR
		FBox2D WorldMiniMapBounds;
		FSlateBrush WPMinimapBrush;
#endif

		void Construct(const FArguments& InArgs, UWorld* InWorld);

		FVector2D ComputeDesiredSize(float LayoutScaleMultiplier) const override
		{
			// No desired size. Always use maximum available space
			return FVector2D::ZeroVector;
		}

		int32 OnPaint(
			const FPaintArgs& Args,
			const FGeometry& AllottedGeometry,
			const FSlateRect& MyCullingRect,
			FSlateWindowElementList& OutDrawElements,
			int32 LayerId,
			const FWidgetStyle& InWidgetStyle,
			bool bParentEnabled) const override;
		
		FVector2D WorldToWidgetSpace(const FGeometry& Geometry, const FVector2D& WorldLocation) const;
	};
} // namespace OUU::Developer::ActorMapWindow