// Copyright (c) 2025 Jonas Reich & Contributors

#include "EngineUtils.h"
#include "Misc/CanvasGraphPlottingUtils.h"
#include "OUUWorldStatsOverlay.h"
#include "Templates/CircularAggregator.h"

class FObjectCountOverlay : public OUU::Developer::FWorldStatsOverlay
{
public:
	static constexpr int32 NumFramesForBuffer = 100;

	FObjectCountOverlay() : FWorldStatsOverlay(.1)
	{
		auto& ActorStats = GraphStats.AddDefaulted_GetRef();
		ActorStats.Name = TEXT("actors + components");
		ActorStats.DataSeries.Add({Buffer_ActorCounts, FColorList::Green, "actors"});
		ActorStats.DataSeries.Add({Buffer_ActorCounts_Destroyed, FColorList::Red, "actors (destroyed)"});

		auto& ComponentStats = GraphStats.AddDefaulted_GetRef();
		ComponentStats.Name = TEXT("components");
		ComponentStats.DataSeries.Add({Buffer_ComponentCounts, FColorList::White, ""});

		auto& PerActorStats = GraphStats.AddDefaulted_GetRef();
		PerActorStats.Name = TEXT("per actor");
		PerActorStats.Limits.Add({20, FColor::Red, "max"});
		PerActorStats.DataSeries.Add({Buffer_ComponentsPerActor, FColorList::White, ""});
	}

private:
	TCircularAggregator<float> Buffer_ActorCounts{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_ActorCounts_Destroyed{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_ComponentCounts{NumFramesForBuffer};
	TCircularAggregator<float> Buffer_ComponentsPerActor{NumFramesForBuffer};

	// - TWorldStatsOverlay
	void TickStats(UWorld* TargetWorld) override
	{
		int32 ActorCount = 0;
		int32 ActorsBeingDestroyedCount = 0;
		int32 ComponentCount = 0;
		for (const auto* Actor : TActorRange<AActor>(TargetWorld))
		{
			++ActorCount;
			ComponentCount += Actor->GetComponents().Num();
			if (Actor->IsPendingKillPending())
			{
				++ActorsBeingDestroyedCount;
			}
		}
		Buffer_ActorCounts.Add(ActorCount);
		Buffer_ActorCounts_Destroyed.Add(ActorsBeingDestroyedCount);
		Buffer_ComponentCounts.Add(ComponentCount);
		Buffer_ComponentsPerActor.Add(static_cast<float>(ComponentCount) / static_cast<float>(ActorCount));
	}
};

DEFINE_OUU_WORLD_STAT_OVERLAY(
	FObjectCountOverlay,
	"ouu.Debug.ObjectCountTest",
	"Display stats of actor + component counts")
