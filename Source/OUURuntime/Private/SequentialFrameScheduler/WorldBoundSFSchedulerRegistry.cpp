// Copyright (c) 2021 Jonas Reich

#include "SequentialFrameScheduler/WorldBoundSFSchedulerRegistry.h"

#include "EngineUtils.h"

AWorldBoundSFSchedulerRegistry::AWorldBoundSFSchedulerRegistry()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	PrimaryActorTick.TickGroup = TG_PrePhysics;

	DuringPhysicsTickFunction.bCanEverTick = true;
	DuringPhysicsTickFunction.bStartWithTickEnabled = true;
	DuringPhysicsTickFunction.TickGroup = TG_DuringPhysics;

	PostPhysicsTickFunction.bCanEverTick = true;
	PostPhysicsTickFunction.bStartWithTickEnabled = true;
	PostPhysicsTickFunction.TickGroup = TG_PostPhysics;
}

bool AWorldBoundSFSchedulerRegistry::FPrioritizedScheduler::operator<(const FPrioritizedScheduler& Other) const
{
	return Priority < Other.Priority;
}

AWorldBoundSFSchedulerRegistry::FSchedulerPtr AWorldBoundSFSchedulerRegistry::GetDefaultScheduler(const UObject* WorldContextObject)
{
	return GetNamedScheduler(WorldContextObject, TEXT("Default"), TG_PrePhysics);
}

AWorldBoundSFSchedulerRegistry::FSchedulerPtr AWorldBoundSFSchedulerRegistry::GetNamedScheduler(const UObject* WorldContextObject, FName SchedulerName, ETickingGroup TickingGroup)
{
	AWorldBoundSFSchedulerRegistry* TypedThis = GetWorldSingleton(WorldContextObject);
	if (!IsValid(TypedThis))
		return nullptr;

	if (FSchedulerPtr* Scheduler = TypedThis->SchedulersByName.Find(SchedulerName))
	{
		check(Scheduler->IsValid());
		return *Scheduler;
	}
	const FSchedulerPtr NewScheduler = MakeShared<FPrioritizedScheduler>();
	TypedThis->TickGroupToSchedulerPriorityList.FindOrAdd(TickingGroup).Add(NewScheduler);
	TypedThis->SchedulersByName.Add(SchedulerName, NewScheduler);
	NewScheduler->Name = SchedulerName;
	return NewScheduler;
}

void AWorldBoundSFSchedulerRegistry::RegisterActorTickFunctions(bool bRegister)
{
	Super::RegisterActorTickFunctions(bRegister);

	if (bRegister)
	{
		if (DuringPhysicsTickFunction.bCanEverTick)
		{
			DuringPhysicsTickFunction.Target = this;
			DuringPhysicsTickFunction.SetTickFunctionEnable(DuringPhysicsTickFunction.bStartWithTickEnabled || DuringPhysicsTickFunction.IsTickFunctionEnabled());
			DuringPhysicsTickFunction.RegisterTickFunction(GetLevel());
		}

		if (DuringPhysicsTickFunction.bCanEverTick)
		{
			PostPhysicsTickFunction.Target = this;
			PostPhysicsTickFunction.SetTickFunctionEnable(PostPhysicsTickFunction.bStartWithTickEnabled || PostPhysicsTickFunction.IsTickFunctionEnabled());
			PostPhysicsTickFunction.RegisterTickFunction(GetLevel());
		}
	}
	else
	{
		if (DuringPhysicsTickFunction.IsTickFunctionRegistered())
		{
			DuringPhysicsTickFunction.UnRegisterTickFunction();
		}

		if (PostPhysicsTickFunction.IsTickFunctionRegistered())
		{
			PostPhysicsTickFunction.UnRegisterTickFunction();
		}
	}
}

void AWorldBoundSFSchedulerRegistry::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	if (auto* QueuePtr = TickGroupToSchedulerPriorityList.Find(ThisTickFunction.TickGroup))
	{
		TArray<FSchedulerPtr>& Queue = *QueuePtr;
		Queue.RemoveAll([](const FSchedulerPtr Scheduler) -> bool
		{
			return !Scheduler.IsValid();
		});
		Queue.Sort([](const FSchedulerPtr A, const FSchedulerPtr B) -> bool
		{
			return A.Get() < B.Get();
		});

		for (FSchedulerPtr Scheduler : Queue)
		{
			Scheduler->Tick(DeltaTime);
		}
	}
}

AWorldBoundSFSchedulerRegistry* AWorldBoundSFSchedulerRegistry::GetWorldSingleton(const UObject* WorldContextObject)
{
	check(IsValid(WorldContextObject));
	UWorld* World = WorldContextObject->GetWorld();
	if (const auto Itr = TActorIterator<AWorldBoundSFSchedulerRegistry>(World))
	{
		return *Itr;
	}
	auto* NewRegistry = World->SpawnActor<AWorldBoundSFSchedulerRegistry>();
	return NewRegistry;
}
