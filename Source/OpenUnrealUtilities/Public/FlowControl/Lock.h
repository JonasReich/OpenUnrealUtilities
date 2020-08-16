// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Lock.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExclusiveLockStateChanged, UExclusiveLock*, Lock, bool, bIsLocked);

/**
 * Object lock that only allows one object to access a resource exclusively
 */
UCLASS()
class OPENUNREALUTILITIES_API UExclusiveLock : public UObject
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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSharedLockStateChanged, USharedLock*, Lock, bool, bIsLocked);

/**
 * Object lock that allows multiple objects to lock a resource.
 * The lock is released as soon as the last key owner has seized control.
 * Possible application: Pause game requests
 */
UCLASS()
class OPENUNREALUTILITIES_API USharedLock : public UObject
{
	GENERATED_BODY()
public:
	/** Called whenever the lock state changes (from unlocked to locked or vice versa) */
	UPROPERTY(BlueprintAssignable)
	FOnSharedLockStateChanged OnLockStateChanged;
	
	/**
	 * Add a key to the lock. May successively with multiple different key objects.
	 * All of these key objects need to removed via TryRelease() in order to release the entire lock.
	 * May be called multiple times with the same key object without any side-effects.
	 */
	UFUNCTION(BlueprintCallable)
	void Lock(UObject* Key);

	/**
	 * Release a single key from the lock. May release the entire lock if it was the last active key.
	 * @param Key: Key to release from the lock
	 * @returns if the entire lock was successfully released
	 */
	UFUNCTION(BlueprintCallable)
	bool TryRelease(UObject* Key);

	/**
	 * Simple check if the lock has any active keys.
	 * Does not check for stale keys!
	 */
	UFUNCTION(BlueprintPure)
	bool IsLocked() const;

	/**
	 * Check if the lock has any active non-stale keys.
	 * Cleans up any stale keys from the internal key list.
	 */
	UFUNCTION(BlueprintPure)
	bool CheckIsLocked();

	/**
	 * Get a copy of the key list. May contain stale entries.
	 * Entries are not stably sorted.
	 */
	UFUNCTION(BlueprintPure)
	TArray<UObject*> GetActiveKeys() const;

private:
	/**
	 * List of active keys. May contain stale entries.
	 * Entries are not stably sorted.
	 */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<UObject>> ActiveKeys;
};
