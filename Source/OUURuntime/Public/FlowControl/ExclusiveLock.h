// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "ExclusiveLock.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExclusiveLockStateChanged, UExclusiveLock*, Lock, bool, bIsLocked);

/**
 * Object lock that only allows one object to access a resource exclusively
 */
UCLASS()
class OUURUNTIME_API UExclusiveLock : public UObject
{
	GENERATED_BODY()
public:
	/** Called whenever the lock state changes (from unlocked to locked or vice versa) */
	UPROPERTY(BlueprintAssignable)
	FOnExclusiveLockStateChanged OnLockStateChanged;

	/**
	 * Attempt to lock the exclusive lock. Will fail if the lock was already locked by a different object.
	 * Calling this function again with the active object key will result in a success without any side-effects.
	 * @returns whether the lock was successfully locked by this key object.
	 */
	UFUNCTION(BlueprintCallable)
	bool TryLock(UObject* Key);

	/**
	 * Attempt to lock the exclusive lock. Will fail if the lock was already locked by a different object.
	 * Calling this function again with the active object key will result in a success without any side-effects.
	 * The lock will be automatically released after the specified time (in game time).
	 * @returns whether the lock was successfully locked by this key object.
	 */
	UFUNCTION(BlueprintCallable)
	bool TryLockForDuration(UObject* Key, float Duration);

	/**
	 * Release the lock with the object which was used to lock it.
	 * Calling unlock with an object that was not used to lock it will trigger an ensure condition.
	 */
	UFUNCTION(BlueprintCallable)
	bool TryUnlock(UObject* Key);

	/** Is the lock locked by a valid key object? */
	UFUNCTION(BlueprintPure)
	bool IsLocked() const;

private:
	/** Active key/owner of the lock. May turn stale while set. */
	UPROPERTY(Transient)
	TWeakObjectPtr<UObject> ActiveKey;
};
