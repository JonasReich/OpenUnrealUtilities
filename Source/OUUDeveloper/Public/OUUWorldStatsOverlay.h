// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/HUD.h"
#include "Misc/CanvasGraphPlottingUtils.h"
#include "Templates/CircularAggregator.h"
#include "Tickable.h"

class AHUD;
class UCanvas;
class APlayerController;

namespace OUU::Developer
{
	class OUUDEVELOPER_API FWorldStatsOverlay : public FTickableGameObject
	{
	protected:
		float UpdateInterval = 0.f;
		float LastUpdateTime = 0.f;
		float AccumulatedTime = 0.f;

		struct FGroupedGraphStats
		{
			FString Name;
			float MaxValue = 0.f;
			TArray<OUU::Runtime::CanvasGraphPlottingUtils::FGraphStatData> DataSeries;
			bool bUseLogarithmicAxis = false;
			TArray<OUU::Runtime::CanvasGraphPlottingUtils::FLimit> Limits;

			void UpdateMaxValue()
			{
				MaxValue = 1.f;
				// make sure limits are always in-frame
				for (const auto& Limit : Limits)
				{
					MaxValue = FMath::Max(MaxValue, Limit.Value);
				}
				for (const auto& Stat : DataSeries)
				{
					MaxValue = FMath::Max(
						MaxValue,
						static_cast<const TCircularAggregator<float>*>(Stat.ValueAggregator.ValueContainer)->Max());
				}
			}
		};

		TArray<FGroupedGraphStats> GraphStats;

	private:
		FDelegateHandle EditorDebugViewDelegate;

	public:
		FWorldStatsOverlay(float InUpdateInterval) : UpdateInterval(InUpdateInterval) {}

		static UWorld* GetStatsWorld();

		virtual void TickStats(UWorld* TargetWorld) = 0;

		void RegisterDebugDrawDelegates(bool Register);

	protected:
		// - FTickableGameObject
		void Tick(float DeltaTime) override;
		TStatId GetStatId() const override;
		ETickableTickType GetTickableTickType() const override;
		bool IsTickableWhenPaused() const override;
		bool IsTickableInEditor() const override;
		UWorld* GetTickableGameObjectWorld() const override;
		// --

		void OnDrawDebugService(UCanvas* InCanvas, APlayerController* PlayerController) const;
		void OnDrawDebug(UCanvas* InCanvas) const;
	};
} // namespace OUU::Developer

#define DEFINE_OUU_WORLD_STAT_OVERLAY(Class, ToggleCVarCommand, ToggleCVarDescription)                                 \
	TUniquePtr<Class> G##Class##Overlay;                                                                               \
	static FAutoConsoleCommand G##Class##ToggleCommand(                                                                \
		TEXT(ToggleCVarCommand),                                                                                       \
		TEXT(ToggleCVarDescription),                                                                                   \
		FConsoleCommandDelegate::CreateStatic([]() {                                                                   \
			if (G##Class##Overlay.IsValid())                                                                           \
			{                                                                                                          \
				G##Class##Overlay->RegisterDebugDrawDelegates(false);                                                  \
				G##Class##Overlay.Reset();                                                                             \
			}                                                                                                          \
			else                                                                                                       \
			{                                                                                                          \
				G##Class##Overlay = MakeUnique<Class>();                                                               \
				G##Class##Overlay->RegisterDebugDrawDelegates(true);                                                   \
			}                                                                                                          \
		}));
