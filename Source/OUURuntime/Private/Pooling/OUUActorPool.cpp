// Copyright (c) 2023 Jonas Reich & Contributors

#include "Pooling/OUUActorPool.h"

#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "LogOpenUnrealUtilities.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Templates/InterfaceUtils.h"
#include "VisualLogger/VisualLogger.h"

CSV_DEFINE_CATEGORY(OUUActorPool, true);

namespace OUU::Runtime::ActorPool
{
	// default values taken from UMassSimulationSettings

	static auto CVar_MaxSpawnTime = TAutoConsoleVariable<float>(
		TEXT("ouu.ActorPool.MaxSpawnTimePerTick"),
		0.0015,
		TEXT("The desired budget in seconds allowed to do pooled actor spawning per frame"));

	static auto CVar_MaxDestructTime = TAutoConsoleVariable<float>(
		TEXT("ouu.ActorPool.MaxDestructTimePerTick"),
		0.0005,
		TEXT("The desired budget in seconds allowed to do pooled actor destruction per frame"));
} // namespace OUU::Runtime::ActorPool

UOUUActorPool* UOUUActorPool::Get(const UObject& WorldContext)
{
	return WorldContext.GetWorld()->GetSubsystem<UOUUActorPool>();
}

UOUUActorPool::FSpawnRequestHandle UOUUActorPool::RequestActorSpawn(const UOUUActorPool::FSpawnRequest& InSpawnRequest)
{
	// The handle manager has a freelist of the release indexes, so it can return us a index that we previously used.
	const auto SpawnRequestHandle = SpawnRequestHandleManager.GetNextHandle();
	const int32 Index = SpawnRequestHandle.GetIndex();

	// Check if we need to grow the array, otherwise it is a previously released index that was returned.
	if (!SpawnRequests.IsValidIndex(Index))
	{
		checkf(
			SpawnRequests.Num() == Index,
			TEXT("This case should only be when we need to grow the array of one element."));
		SpawnRequests.Emplace(InSpawnRequest);
	}
	else
	{
		SpawnRequests[Index] = InSpawnRequest;
	}

	const UWorld* World = GetWorld();
	check(World);

	// Initialize the spawn request status
	auto& SpawnRequest = GetMutableSpawnRequest(SpawnRequestHandle);
	SpawnRequest.Status = ESpawnRequestStatus::Pending;
	SpawnRequest.SerialNumber = RequestSerialNumberCounter.fetch_add(1);
	SpawnRequest.RequestedTime = World->GetTimeSeconds();

	return SpawnRequestHandle;
}

void UOUUActorPool::RetryActorSpawnRequest(const UOUUActorPool::FSpawnRequestHandle SpawnRequestHandle)
{
	check(SpawnRequestHandleManager.IsValidHandle(SpawnRequestHandle));
	const int32 Index = SpawnRequestHandle.GetIndex();
	check(SpawnRequests.IsValidIndex(Index));
	FSpawnRequest& SpawnRequest = SpawnRequests[SpawnRequestHandle.GetIndex()];
	if (ensureMsgf(SpawnRequest.Status == ESpawnRequestStatus::Failed, TEXT("Can only retry failed spawn requests")))
	{
		const UWorld* World = GetWorld();
		check(World);

		SpawnRequest.Status = ESpawnRequestStatus::RetryPending;
		SpawnRequest.SerialNumber = RequestSerialNumberCounter.fetch_add(1);
		SpawnRequest.RequestedTime = World->GetTimeSeconds();
	}
}

bool UOUUActorPool::CancelActorSpawnRequest(UOUUActorPool::FSpawnRequestHandle& SpawnRequestHandle)
{
	if (!ensureMsgf(SpawnRequestHandleManager.RemoveHandle(SpawnRequestHandle), TEXT("Invalid spawn request handle")))
	{
		return false;
	}

	check(SpawnRequests.IsValidIndex(SpawnRequestHandle.GetIndex()));
	FSpawnRequest& SpawnRequest = SpawnRequests[SpawnRequestHandle.GetIndex()];
	check(SpawnRequest.Status != ESpawnRequestStatus::Processing);
	SpawnRequestHandle.Invalidate();
	SpawnRequest.Reset();
	return true;
}

void UOUUActorPool::DestroyOrReleaseActor(AActor* Actor, bool bImmediate)
{
	if (bImmediate)
	{
		if (!TryReleaseActorToPool(Actor))
		{
			UWorld* World = GetWorld();
			check(World);

			World->DestroyActor(Actor);
			--NumActorSpawned;
		}
	}
	else
	{
		ActorsToDestroy.Add(Actor);
	}
}

void UOUUActorPool::DestroyAllActors()
{
	if (UWorld* World = GetWorld())
	{
		for (auto It = PooledActors.CreateIterator(); It; ++It)
		{
			TArray<TObjectPtr<AActor>>& ActorArray = It.Value();
			for (int i = 0; i < ActorArray.Num(); i++)
			{
				World->DestroyActor(ActorArray[i]);
			}
			NumActorSpawned -= ActorArray.Num();
		}
	}
	PooledActors.Empty();

	NumActorPooled = 0;
	CSV_CUSTOM_STAT(OUUActorPool, NumSpawned, NumActorSpawned, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(OUUActorPool, NumPooled, NumActorPooled, ECsvCustomStatOp::Accumulate);
}

const UOUUActorPool::FSpawnRequest& UOUUActorPool::GetSpawnRequest(const FSpawnRequestHandle SpawnRequestHandle) const
{
	check(SpawnRequestHandleManager.IsValidHandle(SpawnRequestHandle));
	check(SpawnRequests.IsValidIndex(SpawnRequestHandle.GetIndex()));
	return SpawnRequests[SpawnRequestHandle.GetIndex()];
}

UOUUActorPool::FSpawnRequest& UOUUActorPool::GetMutableSpawnRequest(const FSpawnRequestHandle SpawnRequestHandle)
{
	check(SpawnRequestHandleManager.IsValidHandle(SpawnRequestHandle));
	check(SpawnRequests.IsValidIndex(SpawnRequestHandle.GetIndex()));
	return SpawnRequests[SpawnRequestHandle.GetIndex()];
}

bool UOUUActorPool::ShouldCreateSubsystem(UObject* Outer) const
{
	// Only create an instance if there is no derived implementation defined elsewhere
	TArray<UClass*> ChildClasses;
	GetDerivedClasses(GetClass(), ChildClasses, false);
	if (ChildClasses.Num() > 0)
	{
		return false;
	}

	return Super::ShouldCreateSubsystem(Outer);
}

void UOUUActorPool::Tick(float DeltaTime)
{
	ProcessPendingDestruction(
		static_cast<double>(OUU::Runtime::ActorPool::CVar_MaxDestructTime.GetValueOnGameThread()));
	ProcessPendingSpawningRequest(
		static_cast<double>(OUU::Runtime::ActorPool::CVar_MaxSpawnTime.GetValueOnGameThread()));
	CSV_CUSTOM_STAT(OUUActorPool, NumSpawned, NumActorSpawned, ECsvCustomStatOp::Accumulate);
	CSV_CUSTOM_STAT(OUUActorPool, NumPooled, NumActorPooled, ECsvCustomStatOp::Accumulate);
}

TStatId UOUUActorPool::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FOUUActorPool, STATGROUP_Quick)
}

void UOUUActorPool::AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector)
{
	if (auto* CastedThis = Cast<UOUUActorPool>(InThis))
	{
		for (auto& Entry : CastedThis->PooledActors)
		{
			Collector.AddReferencedObjects<AActor>(Entry.Value);
		}
	}

	Super::AddReferencedObjects(InThis, Collector);
}

UOUUActorPool::FSpawnRequestHandle UOUUActorPool::GetNextRequestToSpawn() const
{
	FSpawnRequestHandle BestSpawnRequestHandle;
	float BestPriority = MAX_FLT;
	bool bBestIsPending = false;
	uint32 BestSerialNumber = MAX_uint32;
	for (const FSpawnRequestHandle SpawnRequestHandle : SpawnRequestHandleManager.GetHandles())
	{
		if (!SpawnRequestHandle.IsValid())
		{
			continue;
		}
		const FSpawnRequest& SpawnRequest = GetSpawnRequest(SpawnRequestHandle);
		if (SpawnRequest.Status == ESpawnRequestStatus::Pending)
		{
			if (!bBestIsPending || SpawnRequest.Priority < BestPriority
				|| (SpawnRequest.Priority == BestPriority && SpawnRequest.SerialNumber < BestSerialNumber))
			{
				BestSpawnRequestHandle = SpawnRequestHandle;
				BestSerialNumber = SpawnRequest.SerialNumber;
				BestPriority = SpawnRequest.Priority;
				bBestIsPending = true;
			}
		}
		else if (!bBestIsPending && SpawnRequest.Status == ESpawnRequestStatus::RetryPending)
		{
			// No priority on retries just FIFO
			if (SpawnRequest.SerialNumber < BestSerialNumber)
			{
				BestSpawnRequestHandle = SpawnRequestHandle;
				BestSerialNumber = SpawnRequest.SerialNumber;
			}
		}
	}

	return BestSpawnRequestHandle;
}

AActor* UOUUActorPool::SpawnOrRetrieveFromPool(
	const FSpawnRequestHandle SpawnRequestHandle,
	FSpawnRequest& SpawnRequest)
{
	TArray<TObjectPtr<AActor>>* Pool = PooledActors.Find(SpawnRequest.Template);

	if (Pool && Pool->Num() > 0)
	{
		TObjectPtr<AActor> PooledActor = (*Pool)[0];
		Pool->RemoveAt(0);
		--NumActorPooled;
		ActivateActor(PooledActor);
		PooledActor->SetActorTransform(SpawnRequest.Transform, false, nullptr, ETeleportType::ResetPhysics);

		CALL_INTERFACE(IOUUPoolableActor, OnRemovedFromPool, PooledActor);

		return PooledActor;
	}
	return SpawnActor(SpawnRequestHandle, SpawnRequest);
}

AActor* UOUUActorPool::SpawnActor(const FSpawnRequestHandle SpawnRequestHandle, FSpawnRequest SpawnRequest) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UActorPool::SpawnActor);

	UWorld* World = GetWorld();
	check(World);

	if (AActor* SpawnedActor = World->SpawnActorDeferred<AActor>(
			SpawnRequest.Template,
			SpawnRequest.Transform,
			nullptr,
			nullptr,
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn))
	{
		// Call construction script
		if (SpawnRequest.DelayFirstConstructionScript == false)
		{
			SpawnedActor->FinishSpawning(SpawnRequest.Transform);
		}
		++NumActorSpawned;
		// The finish spawning might have failed and the spawned actor is destroyed.
		if (IsValidChecked(SpawnedActor))
		{
			return SpawnedActor;
		}
	}

	UE_VLOG_CAPSULE(
		this,
		LogOpenUnrealUtilities,
		Error,
		SpawnRequest.Transform.GetLocation(),
		SpawnRequest.Template.GetDefaultObject()->GetSimpleCollisionHalfHeight(),
		SpawnRequest.Template.GetDefaultObject()->GetSimpleCollisionRadius(),
		SpawnRequest.Transform.GetRotation(),
		FColor::Red,
		TEXT("Unable to spawn actor entity [%s]"),
		*GetNameSafe(SpawnRequest.Template));
	return nullptr;
}

void UOUUActorPool::ActivateActor(AActor* Actor) const
{
	Actor->SetActorHiddenInGame(false);
}

void UOUUActorPool::DeactivateActor(AActor* Actor) const
{
	Actor->SetActorEnableCollision(false);
	Actor->SetActorHiddenInGame(true);
	Actor->SetActorTickEnabled(false);
}

void UOUUActorPool::DeactivateActorFast(AActor* Actor) const
{
	Actor->SetActorHiddenInGame(true);
}

void UOUUActorPool::ProcessPendingSpawningRequest(const double MaxTimeSlicePerTick)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UActorPool::ProcessPendingSpawningRequest);
	SpawnRequestHandleManager.ShrinkHandles();

	const double TimeSliceEnd = FPlatformTime::Seconds() + MaxTimeSlicePerTick;

	while (FPlatformTime::Seconds() < TimeSliceEnd)
	{
		auto SpawnRequestHandle = GetNextRequestToSpawn();
		if (!SpawnRequestHandle.IsValid()
			|| !ensureMsgf(
				SpawnRequestHandleManager.IsValidHandle(SpawnRequestHandle),
				TEXT("GetNextRequestToSpawn returned an invalid handle, expecting an empty one or a valid one.")))
		{
			return;
		}

		auto SpawnRequest = SpawnRequests[SpawnRequestHandle.GetIndex()];

		if (!ensureMsgf(
				SpawnRequest.Status == ESpawnRequestStatus::Pending
					|| SpawnRequest.Status == ESpawnRequestStatus::RetryPending,
				TEXT("GetNextRequestToSpawn returned a request that was already processed, need to return only request "
					 "with pending status.")))
		{
			return;
		}

		// Do the spawning
		SpawnRequest.Status = ESpawnRequestStatus::Processing;

		SpawnRequest.SpawnedActor = SpawnOrRetrieveFromPool(SpawnRequestHandle, SpawnRequest);

		SpawnRequest.Status = SpawnRequest.SpawnedActor ? ESpawnRequestStatus::Succeeded : ESpawnRequestStatus::Failed;

		// Call the post spawn delegate on the spawn request
		if (SpawnRequest.PostSpawnDelegate.IsBound())
		{
			if (SpawnRequest.PostSpawnDelegate.Execute(SpawnRequestHandle, SpawnRequest)
				== EOUUActorPoolSpawnRequestAction::Remove)
			{
				SpawnRequestHandleManager.RemoveHandle(SpawnRequestHandle);
			}
		}
		// ... or retry
		else if (SpawnRequest.RetryIndefinitely == true && SpawnRequest.SpawnedActor == nullptr)
		{
			RetryActorSpawnRequest(SpawnRequestHandle);
		}
		// ... or remove
		else
		{
			SpawnRequestHandleManager.RemoveHandle(SpawnRequestHandle);
		}
	}
}

void UOUUActorPool::ProcessPendingDestruction(const double MaxTimeSlicePerTick)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(UActorPool::ProcessPendingDestruction);

	UWorld* World = GetWorld();
	check(World);

	{
		const ENetMode CurrentWorldNetMode = World->GetNetMode();
		const double HasToDestroyAllActorsOnServerSide =
			CurrentWorldNetMode != NM_Client && CurrentWorldNetMode != NM_Standalone;
		const double TimeSliceEnd = FPlatformTime::Seconds() + MaxTimeSlicePerTick;

		// Try release to pool actors or destroy them
		TRACE_CPUPROFILER_EVENT_SCOPE(DestroyActors);
		while ((DeactivatedActorsToDestroy.Num() || ActorsToDestroy.Num())
			   && (HasToDestroyAllActorsOnServerSide || FPlatformTime::Seconds() <= TimeSliceEnd))
		{
			AActor* ActorToDestroy = DeactivatedActorsToDestroy.Num()
				? DeactivatedActorsToDestroy.Pop(/*bAllowShrinking*/ false)
				: ActorsToDestroy.Pop(/*bAllowShrinking*/ false);
			if (!TryReleaseActorToPool(ActorToDestroy))
			{
				// Couldn't release actor back to pool, so destroy it
				World->DestroyActor(ActorToDestroy);
				--NumActorSpawned;
			}
		}
	}

	if (ActorsToDestroy.Num())
	{
		// Try release to pool remaining actors or deactivate them
		TRACE_CPUPROFILER_EVENT_SCOPE(DeactivateActors);
		for (AActor* ActorToDestroy : ActorsToDestroy)
		{
			if (!TryReleaseActorToPool(ActorToDestroy))
			{
				// Couldn't release actor back to pool this frame -> do a simple (hopefully time saving) deactivation
				// and queue the actor destroy/deactivate for next frame.
				DeactivateActorFast(ActorToDestroy);
				DeactivatedActorsToDestroy.Add(ActorToDestroy);
			}
		}
		ActorsToDestroy.Reset();
	}
}

bool UOUUActorPool::TryReleaseActorToPool(AActor* Actor)
{
	const bool bIsPoolableActor = IsValidInterface<IOUUPoolableActor>(Actor);
	if (bIsPoolableActor && CALL_INTERFACE(IOUUPoolableActor, CanBePooled, Actor))
	{
		TArray<TObjectPtr<AActor>>& Pool = PooledActors.FindOrAdd(Actor->GetClass());

		const int32 MaxPoolSize = CALL_INTERFACE(IOUUPoolableActor, GetMaxPoolSize, Actor);
		if (Pool.Num() >= MaxPoolSize)
			return false;

		CALL_INTERFACE(IOUUPoolableActor, OnAddedToPool, Actor);

		DeactivateActor(Actor);

		checkf(Pool.Find(Actor) == INDEX_NONE, TEXT("Actor %s is already in the pool"), *AActor::GetDebugName(Actor));
		Pool.Add(Actor);
		++NumActorPooled;
		return true;
	}
	return false;
}
