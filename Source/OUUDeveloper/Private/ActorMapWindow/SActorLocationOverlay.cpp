// Copyright (c) 2026 Jonas Reich & Contributors

#include "SActorLocationOverlay.h"

#include "Fonts/FontMeasure.h"
#include "OUUActorMapStyle.h"
#include "Polygon2.h"

#if WITH_EDITOR
	#include "WorldPartition/WorldPartitionMiniMap.h"
	#include "WorldPartition/WorldPartitionMiniMapHelper.h"
#endif

namespace OUU::Developer::ActorMapWindow
{
	// original: 6.f
	constexpr float MarkerSize = 2.f;

	void SActorLocationOverlay::Construct(const FArguments& InArgs, UWorld* InWorld)
	{
		ActorQueries = InArgs._ActorQueries;
		ReferencePosition = InArgs._ReferencePosition;
		MapSize = InArgs._MapSize;
		ShowFlags = InArgs._ShowFlags;

#if WITH_EDITOR
		if (AWorldPartitionMiniMap* WorldMiniMap = FWorldPartitionMiniMapHelper::GetWorldPartitionMiniMap(InWorld))
		{
			WorldMiniMapBounds = FBox2D(
				FVector2D(WorldMiniMap->MiniMapWorldBounds.Min),
				FVector2D(WorldMiniMap->MiniMapWorldBounds.Max));
			if (UTexture2D* MiniMapTexture = WorldMiniMap->MiniMapTexture)
			{
				WPMinimapBrush.SetUVRegion(WorldMiniMap->UVOffset);
				WPMinimapBrush.SetImageSize(MiniMapTexture->GetImportedSize());
				WPMinimapBrush.SetResourceObject(MiniMapTexture);
			}
		}
#endif
	}

	int32 SActorLocationOverlay::OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const
	{
#if WITH_EDITOR
		const FBox2D MinimapBounds(
			WorldToWidgetSpace(AllottedGeometry, WorldMiniMapBounds.Min),
			WorldToWidgetSpace(AllottedGeometry, WorldMiniMapBounds.Max));

		const FPaintGeometry WorldImageGeometry =
			AllottedGeometry.ToPaintGeometry(MinimapBounds.GetSize(), FSlateLayoutTransform(MinimapBounds.Min));

		// only show WP minimap if we have a minimap and no scene capture.
		const bool bShowSceneCapture(ShowFlags.Get() & EShowFlags::SceneCapture);
		if (bShowSceneCapture == false && IsValid(Cast<UTexture2D>(WPMinimapBrush.GetResourceObject())))
		{
			FSlateDrawElement::MakeBox(
				OutDrawElements,
				++LayerId,
				WorldImageGeometry,
				&WPMinimapBrush,
				ESlateDrawEffect::None,
				FLinearColor::White);
		}

		static FSlateColorBrush BackgroundTintBrush(FLinearColor::White);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			++LayerId,
			WorldImageGeometry,
			&BackgroundTintBrush,
			ESlateDrawEffect::None,
			FLinearColor(0, 0, 0, 0.25));
#endif

		if (ActorQueries.Get() == nullptr)
			return LayerId;

		// Used to track the layer ID we will return.
		int32 RetLayerId = LayerId;

		const bool bEnabled = ShouldBeEnabled(bParentEnabled);
		const ESlateDrawEffect DrawEffects = bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;

		const FVector2D LocalSize = AllottedGeometry.GetLocalSize();
		const FVector2D LocalCenter = LocalSize / 2;
		const float MaxComponent = LocalCenter.GetMin();
		const FVector2D CenterOffset = FVector2D(-MaxComponent, -MaxComponent);
		const FVector2D Position = LocalCenter + CenterOffset;
		const FVector2D Size = FVector2D(MaxComponent * 2.f, MaxComponent * 2.f);

		const float MapSizeActual = MapSize.Get();
		const FVector HalfMapSizeVector = FVector(MapSizeActual / 2.f, MapSizeActual / 2.f, 0);
		const FVector TopLeftCorner = ReferencePosition.Get() - HalfMapSizeVector;
		FBox BBox(TopLeftCorner, ReferencePosition.Get() + HalfMapSizeVector);

		const TSharedRef<FSlateFontMeasure> FontMeasure =
			FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		auto& Style = FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>("SmallText");
		const FSlateFontInfo FontInfo = Style.Font;

		auto DrawDistanceCircle = [&](float CircleRadiusMultiplier) {
			const float CircleRadius = MaxComponent * CircleRadiusMultiplier;
			UE::Geometry::FPolygon2f PathCircle = UE::Geometry::FPolygon2f::MakeCircle(CircleRadius, 32);
			// append a vertex copy. Otherwise we get a crash, because the internal array gets resized and moved
			// before resolving the array reference
			const auto FirstVertexCopy = CopyTemp(PathCircle.GetVertices()[0]);
			PathCircle.AppendVertex(FirstVertexCopy);
			FSlateDrawElement::MakeLines(
				OutDrawElements,
				RetLayerId++,
				AllottedGeometry.ToPaintGeometry(Size, FSlateLayoutTransform(1.f, Position + Size / 2.0)),
				PathCircle.GetVertices(),
				DrawEffects);

			const float CircleRadiusWorldScale = (MapSizeActual / 2.f) * CircleRadiusMultiplier;
			constexpr double WorldToMeters = 100.0;
			FSlateDrawElement::MakeText(
				OutDrawElements,
				RetLayerId++,
				AllottedGeometry.ToPaintGeometry(
					FVector2D(FVector2f(1.0, 1.0)),
					FSlateLayoutTransform(1.f, Position + FVector2D(MaxComponent - CircleRadius, Size.Y / 2.0))),
				FText::AsCultureInvariant(FString::Printf(TEXT("%.1fm"), CircleRadiusWorldScale / WorldToMeters)),
				FontInfo,
				DrawEffects);
		};

		if (ShowFlags.Get() & EShowFlags::Distance)
		{
			DrawDistanceCircle(1.f);
			DrawDistanceCircle(1.f / 2.f);
			DrawDistanceCircle(1.f / 4.f);
			DrawDistanceCircle(1.f / 8.f);
		}

		auto ActualActorQueries = *ActorQueries.Get();
		for (auto Query : ActualActorQueries)
		{
			if (Query.IsValid() == false || Query->CachedResult == nullptr)
				continue;

			for (auto& ActorEntry : *Query->CachedResult)
			{
				auto [WorldLocation, Label] = ActorEntry;

				if (!BBox.IsInsideXY(WorldLocation))
					continue;

				const FVector2D WidgetSpaceLocation =
					WorldToWidgetSpace(AllottedGeometry, {WorldLocation.X, WorldLocation.Y});

				FSlateDrawElement::MakeBox(
					OutDrawElements,
					RetLayerId++,
					AllottedGeometry.ToPaintGeometry(
						FVector2D(MarkerSize, MarkerSize),
						FSlateLayoutTransform(1.f, WidgetSpaceLocation - (MarkerSize / 2.f))),
					&Style::White,
					DrawEffects,
					Query->QueryColor);

				if (this->ShowFlags.Get() & EShowFlags::Labels)
				{
					FText LabelText = FText::AsCultureInvariant(Label);

					FVector2f LabelDimensions = FontMeasure->Measure(LabelText, FontInfo);

					FSlateDrawElement::MakeBox(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D(LabelDimensions),
							FSlateLayoutTransform(
								1.f,
								WidgetSpaceLocation - (MarkerSize / 2.f) + FVector2D(0, MarkerSize))),
						&Style::White,
						DrawEffects,
						Style::LabelBackgroundColor);

					FSlateDrawElement::MakeText(
						OutDrawElements,
						RetLayerId++,
						AllottedGeometry.ToPaintGeometry(
							FVector2D(FVector2f(MarkerSize, MarkerSize)),
							FSlateLayoutTransform(
								1.f,
								WidgetSpaceLocation - (MarkerSize / 2.f) + FVector2D(0, MarkerSize))),
						LabelText,
						FontInfo,
						DrawEffects,
						Query->QueryColor);
				}
			}
		}

		return RetLayerId - 1;
	}

	FVector2D SActorLocationOverlay::WorldToWidgetSpace(const FGeometry& Geometry, const FVector2D& WorldLocation) const
	{
		const FVector2D LocalCenter = Geometry.GetLocalSize() / 2;
		const float MaxComponent = LocalCenter.GetMin();
		const FVector2D CenterOffset(-MaxComponent, -MaxComponent);
		const FVector2D Position = LocalCenter + CenterOffset;
		const FVector2D Size(MaxComponent * 2.f, MaxComponent * 2.f);

		const float MapSizeActual = MapSize.Get();
		const FVector2D HalfMapSizeVector(MapSizeActual / 2.f, MapSizeActual / 2.f);

		const FVector2D ReferencePosition2D(ReferencePosition.Get().X, ReferencePosition.Get().Y);
		const FVector2D TopLeftCorner = ReferencePosition2D - HalfMapSizeVector;
		const FVector2D RelativeLocation2D = WorldLocation - TopLeftCorner;
		const FVector2D RelativeLocation2D_Normalized = RelativeLocation2D / MapSizeActual;

		const FVector2D WidgetSpaceLocationNormalized{RelativeLocation2D_Normalized.X, RelativeLocation2D_Normalized.Y};
		return Position + WidgetSpaceLocationNormalized * Size;
	}

} // namespace OUU::Developer::ActorMapWindow
