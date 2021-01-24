// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "SharedLock.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnSharedLockStateChanged, USharedLock*, Lock, bool, bIsLocked);

/**
 * Object lock that allows multiple objects to lock a resource.
 * The lock is released as soon as the last key owner has seized control.
 * Possible application: Pause game requests
 */
UCLASS()
class OUURUNTIME_API USharedLock : public UObject
{
	GENERATED_BODY()
public:
	/** Called whenever the lock state changes (from unlocked to locked or vice versa) */
	UPROPERTY(BlueprintAssignable)
	FOnSharedLockStateChanged OnLockStateChanged;
	
	/**
	 * Add a key to the lock. May be called successively with multiple different key objects.
	 * All of these key objects need to be removed via TryRelease() in order to release the entire lock.
	 * May be called multiple times with the same key object without any side-effects.
	 */
	UFUNCTION(BlueprintCallable)
	void Lock(UObject* Key);

	/**
	 * Add a key to the lock. May be called successively with multiple different key objects.
	 * The key will be released automatically after the specified time.
	 * All of these key objects need to either be removed via TryRelease() or waited until
	 * the specified duration has passed, so the entire lock is released again.
	 * May be called multiple times with the same key object, which effectively resets the timer.
	 */
	UFUNCTION(BlueprintCallable)
	void LockForDuration(UObject* Key, float Duration);

	/**
	 * Release a single key from the lock. May release the entire lock if it was the last active key.
	 * @param Key: Key to release from the lock
	 * @returns if the entire lock was successfully released
	 */
	UFUNCTION(BlueprintCallable)
	bool TryUnlock(UObject* Key);

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
