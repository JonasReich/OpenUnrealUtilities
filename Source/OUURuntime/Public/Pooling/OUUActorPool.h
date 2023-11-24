// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "IndexedHandle.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/Interface.h"

#include "OUUActorPool.generated.h"

struct FOUUActorPoolSpawnRequest;

UINTERFACE(Blueprintable)
class OUURUNTIME_API UOUUPoolableActor : public UInterface
{
	GENERATED_BODY()
public:
};

class OUURUNTIME_API IOUUPoolableActor : public IInterface
{
	GENERATED_BODY()
public:
	/**
	 * Override in derived classes to check if an actor can be released to the pool.
	 * Actors that are in an irrecoverable state and should not be re-used should return false.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Open Unreal Utilities|Actor Pooling")
	bool CanBePooled() const;
	virtual bool CanBePooled_Implementation() const { return true; }

	/**
	 * Prepare an actor that was stored in pool for game.
	 * For poolable actors this is similar to BeginPlay().
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Open Unreal Utilities|Actor Pooling")
	void OnRemovedFromPool();

	/**
	 * Called when an actor was initially added or returned to pool.
	 * Prepare an actor that was active in-game for pooling.
	 * For poolable actors this is similar to EndPlay().
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Open Unreal Utilities|Actor Pooling")
	void OnAddedToPool();

	/** How many actors can be pooled (inactive) at the same time? Default: 10 */
	UFUNCTION(BlueprintNativeEvent, Category = "Open Unreal Utilities|Actor Pooling")
	int32 GetMaxPoolSize() const;
	int32 GetMaxPoolSize_Implementation() const { return 10; }
};

USTRUCT()
struct OUURUNTIME_API FOUUActorPoolSpawnRequestHandle : public FIndexedHandleBase
{
	GENERATED_BODY()

	FOUUActorPoolSpawnRequestHandle() = default;

	/** @note passing INDEX_NONE as index will make this handle Invalid */
	FOUUActorPoolSpawnRequestHandle(const int32 InIndex, const uint32 InSerialNumber) :
		FIndexedHandleBase(InIndex, InSerialNumber)
	{
	}
};

// Managing class of spawning requests handles
typedef FIndexedHandleManager<FOUUActorPoolSpawnRequestHandle, true /*bOptimizeHandleReuse*/>
	FOUUActorPoolHandleManager_ActorSpawnRequest;

UENUM()
enum class EOUUActorPoolSpawnRequestAction : uint8
{
	Keep,	// Will leave spawning request active and it will be users job to call Cancel or Retry depending on state.
	Remove, // Will remove the spawning request from the queue once the callback ends
};

DECLARE_DELEGATE_RetVal_TwoParams(
	EOUUActorPoolSpawnRequestAction,
	FOUUActorPoolPostSpawnDelegate,
	const FOUUActorPoolSpawnRequestHandle&,
	const FOUUActorPoolSpawnRequest&);

UENUM()
enum class EOUUActorPoolSpawnRequestStatus : uint8
{
	None,		  // Not in the queue to be spawned
	Pending,	  // Still in the queue to be spawned
	Processing,	  // In the process of spawning the actor
	Succeeded,	  // Successfully spawned the actor
	Failed,		  // Error while spawning the actor
	RetryPending, // Waiting to retry after a failed spawn request (lower priority)
};

/**
 * Struct for spawn requests (opposite to Mass system, this is not meant to be subclassed).
 */
USTRUCT()
struct OUURUNTIME_API FOUUActorPoolSpawnRequest
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	TSubclassOf<AActor> Template;

	FTransform Transform;

	// Priority of this spawn request in comparison with the others, the lower the value is, the higher the priority is
	float Priority = MAX_FLT;

	FOUUActorPoolPostSpawnDelegate PostSpawnDelegate;

	EOUUActorPoolSpawnRequestStatus Status = EOUUActorPoolSpawnRequestStatus::None;

	// The pointer to the actor once it is spawned
	UPROPERTY(Transient)
	TObjectPtr<AActor> SpawnedActor = nullptr;

	// Internal request serial number (used to cycle through next spawning request)
	uint32 SerialNumber = 0;

	// Creation world time of the request in seconds
	double RequestedTime = 0.;

	// If retries are enabled, the spawn request is retried indefinitely until it succeeds.
	bool RetryIndefinitely = false;

	// If enabled the spawned actor's construction script is not run and must be executed by user.
	bool DelayFirstConstructionScript = false;

	void Reset()
	{
		Template = nullptr;
		Priority = MAX_FLT;
		PostSpawnDelegate.Unbind();
		Status = EOUUActorPoolSpawnRequestStatus::None;
		SpawnedActor = nullptr;
		SerialNumber = 0;
		RequestedTime = 0.0f;
	}
};

/**
 * Actor pool similar to the Mass Actor Pool, but without the Mass struct utils dependencies and some modifications
 * that made it a bit easier to use with regular actors.
 */
UCLASS()
class OUURUNTIME_API UOUUActorPool : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	using FSpawnRequest = FOUUActorPoolSpawnRequest;
	using FSpawnRequestHandle = FOUUActorPoolSpawnRequestHandle;
	using ESpawnRequestStatus = EOUUActorPoolSpawnRequestStatus;

	static UOUUActorPool* Get(UObject& WorldContext);

	FSpawnRequestHandle RequestActorSpawn(const FSpawnRequest& InSpawnRequest);
	void RetryActorSpawnRequest(const FSpawnRequestHandle SpawnRequestHandle);
	bool CancelActorSpawnRequest(FSpawnRequestHandle& SpawnRequestHandle);

	// Return back to pool and deactivate or destroy
	void DestroyOrReleaseActor(AActor* Actor, bool bImmediate = false);

	// To release all resources
	void DestroyAllActors();

	const FSpawnRequest& GetSpawnRequest(const FSpawnRequestHandle SpawnRequestHandle) const;
	FSpawnRequest& GetMutableSpawnRequest(const FSpawnRequestHandle SpawnRequestHandle);

	// - USubsystem
	bool ShouldCreateSubsystem(UObject* Outer) const;
	// - FTickableGameObject
	void Tick(float DeltaTime) override;
	TStatId GetStatId() const override;
	// - UObject (via reflection)
	static void AddReferencedObjects(UObject* InThis, FReferenceCollector& Collector);
	// --
protected:
	virtual FSpawnRequestHandle GetNextRequestToSpawn() const;
	virtual AActor* SpawnOrRetrieveFromPool(const FSpawnRequestHandle SpawnRequestHandle, FSpawnRequest& SpawnRequest);
	virtual AActor* SpawnActor(const FSpawnRequestHandle SpawnRequestHandle, FSpawnRequest SpawnRequest) const;
	virtual void ActivateActor(AActor* Actor) const;
	virtual void DeactivateActor(AActor* Actor) const;
	/**
	 * Called if too many actors are deactivated in the same frame on any remaining actors.
	 * This should do the minimum version of DeactivateActor().
	 * The full DeactivateActor() implementation is called as soon as possible when the time budget allows it.
	 */
	virtual void DeactivateActorFast(AActor* Actor) const;

private:
	UPROPERTY()
	TArray<FOUUActorPoolSpawnRequest> SpawnRequests;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> ActorsToDestroy;

	UPROPERTY()
	TArray<TObjectPtr<AActor>> DeactivatedActorsToDestroy;

	TMap<TSubclassOf<AActor>, TArray<AActor*>> PooledActors;
	FOUUActorPoolHandleManager_ActorSpawnRequest SpawnRequestHandleManager;
	std::atomic<uint32> RequestSerialNumberCounter;
	mutable int32 NumActorSpawned = 0;
	mutable int32 NumActorPooled = 0;

	void ProcessPendingSpawningRequest(const double MaxTimeSlicePerTick);
	void ProcessPendingDestruction(const double MaxTimeSlicePerTick);
	bool TryReleaseActorToPool(AActor* Actor);
};
