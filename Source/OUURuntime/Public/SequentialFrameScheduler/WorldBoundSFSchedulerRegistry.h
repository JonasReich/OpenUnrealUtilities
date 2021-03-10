// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "SequentialFrameScheduler.h"
#include "WorldBoundSFSchedulerRegistry.generated.h"

/**
 * Registry actor that for FSequentialFrameSchedulers that is bound to the lifetime to the world it was requested in.
 * This means loading a new level will result in the frame scheduler being destroyed.
 * The registry actor will be lazily spawned when it's first requested in a world.
 * You can request multiple different schedulers from the same registry to group tasks of different kinds together.
 * Just like the registry these schedulers will be created on-demand.
 */
UCLASS()
class OUURUNTIME_API AWorldBoundSFSchedulerRegistry : public AInfo
{
	GENERATED_BODY()
public:
#if WITH_GAMEPLAY_DEBUGGER
	friend class FGameplayDebuggerCategory_SequentialFrameScheduler;
#endif

	/** Sequential scheduler with a priority index. Schedulers are ticked in order from highest to lowest priority. */
	struct FPrioritizedScheduler : public FSequentialFrameScheduler
	{
		int32 Priority = 0;
		FName Name = NAME_None;

		bool operator<(const FPrioritizedScheduler& Other) const;
	};

	using FSchedulerPtr = TSharedPtr<FPrioritizedScheduler>;

	AWorldBoundSFSchedulerRegistry();

	/** Get the default scheduler (PrePhysics tick and default name) */
	static FPrioritizedScheduler& GetDefaultScheduler(const UObject* WorldContextObject);
	static FPrioritizedScheduler& GetNamedScheduler(const UObject* WorldContextObject, FName SchedulerName, ETickingGroup TickingGroup);

	// - AActor
	virtual void RegisterActorTickFunctions(bool bRegister) override;
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	// --

private:
	UPROPERTY()
	FActorTickFunction DuringPhysicsTickFunction;

	UPROPERTY()
	FActorTickFunction PostPhysicsTickFunction;

	/** Name map. Used by public API to access schedulers */
	TMap<FName, FSchedulerPtr> SchedulersByName;

	/** Store a priority sorted list for each tick group. Used to propagate Tick() to schedulers */
	TMap<ETickingGroup, TArray<FSchedulerPtr>> TickGroupToSchedulerPriorityList;

	static AWorldBoundSFSchedulerRegistry& GetWorldSingleton(const UObject* WorldContextObject);
};
