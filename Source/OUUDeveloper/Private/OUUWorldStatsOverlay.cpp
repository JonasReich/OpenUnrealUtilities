
#include "OUUWorldStatsOverlay.h"

#include "Debug/DebugDrawService.h"
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "Misc/CanvasGraphPlottingUtils.h"

#if WITH_EDITOR
	#include "Editor.h"
#endif

// Show flag to allow drawing any of the FWorldStatsOverlay instances.
// The EShowFlagShippingValue value should not matter, because this is all part of development code anyways.
TCustomShowFlag<EShowFlagShippingValue::ForceDisabled> GWorldStatsOverlays_ShowFlag(
	TEXT("OUU_WorldStatsOverlays"),
	true,
	EShowFlagGroup::SFG_Custom,
	INVTEXT("OUU World Stats Overlays"));

namespace OUU::Developer
{
	UWorld* FWorldStatsOverlay::GetStatsWorld()
	{
		auto* CurrentPlayWorld = GEngine->GetCurrentPlayWorld();
#if WITH_EDITOR
		if (!CurrentPlayWorld)
		{
			return GEditor->GetEditorWorldContext().World();
		}
#endif
		return CurrentPlayWorld;
	}

	void FWorldStatsOverlay::RegisterDebugDrawDelegates(bool Register)
	{
		if (Register)
		{
			EditorDebugViewDelegate = UDebugDrawService::Register(
				TEXT("OUU_WorldStatsOverlays"),
				FDebugDrawDelegate::CreateRaw(this, &FWorldStatsOverlay::OnDrawDebugService));
		}
		else
		{
			UDebugDrawService::Unregister(EditorDebugViewDelegate);
			EditorDebugViewDelegate.Reset();
		}
	}

	void FWorldStatsOverlay::Tick(float DeltaTime)
	{
		if (auto* TargetWorld = GetStatsWorld())
		{
#if WITH_EDITOR
			if (GEditor->IsPlaySessionInProgress() && TargetWorld->WorldType != EWorldType::PIE)
			{
				return;
			}
#endif

			AccumulatedTime += DeltaTime;
			if (AccumulatedTime < UpdateInterval)
			{
				return;
			}

			TickStats(TargetWorld);

			for (auto& Entry : GraphStats)
			{
				Entry.UpdateMaxValue();
			}

			while (AccumulatedTime > UpdateInterval)
			{
				AccumulatedTime -= UpdateInterval;
			}
		}
	}

	TStatId FWorldStatsOverlay::GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FWorldStatsOverlay, STATGROUP_Tickables);
	}

	ETickableTickType FWorldStatsOverlay::GetTickableTickType() const { return ETickableTickType::Always; }

	bool FWorldStatsOverlay::IsTickableWhenPaused() const { return true; }

	bool FWorldStatsOverlay::IsTickableInEditor() const { return true; }

	UWorld* FWorldStatsOverlay::GetTickableGameObjectWorld() const { return GetStatsWorld(); }

	void FWorldStatsOverlay::OnDrawDebugService(UCanvas* InCanvas, APlayerController* PlayerController) const
	{
		OnDrawDebug(InCanvas);
	}

	void FWorldStatsOverlay::OnDrawDebug(UCanvas* InCanvas) const
	{
		const float GraphGridBottomY =
			InCanvas->Canvas->GetRenderTarget()->GetSizeXY().Y / InCanvas->GetDPIScale() - 50.0f;

		const float CanvasWidth = InCanvas->Canvas->GetRenderTarget()->GetSizeXY().X / InCanvas->GetDPIScale();

		constexpr float OuterPaddingX = 80.0f;
		// this is to keep space for the "stat unitgraph" panel
		constexpr float GraphGridLeftX = OuterPaddingX + 450.0f;
		constexpr float WidthPerGraph = 350.f;
		constexpr float HeightPerGraph = 350.f;

		const float RemainingWidth = (CanvasWidth - GraphGridLeftX - OuterPaddingX);
		const int32 NumGraphsPerRow = FMath::Floor(RemainingWidth / WidthPerGraph);

		for (int32 StatsIndex = 0; StatsIndex < GraphStats.Num(); ++StatsIndex)
		{
			const int32 GraphGrid_X = StatsIndex % NumGraphsPerRow;
			const int32 GraphGrid_Y = StatsIndex / NumGraphsPerRow;

			auto& StatsGroup = GraphStats[StatsIndex];
			OUU::Runtime::CanvasGraphPlottingUtils::DrawCanvasGraph(
				InCanvas->Canvas,
				GraphGridLeftX + GraphGrid_X * WidthPerGraph,
				GraphGridBottomY - GraphGrid_Y * HeightPerGraph,
				StatsGroup.DataSeries,
				StatsGroup.Name,
				StatsGroup.MaxValue,
				StatsGroup.bUseLogarithmicAxis,
				StatsGroup.Limits);
		}
	}
} // namespace OUU::Developer
