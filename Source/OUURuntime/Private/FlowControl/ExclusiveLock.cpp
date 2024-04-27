// Copyright (c) 2023 Jonas Reich & Contributors

#include "FlowControl/ExclusiveLock.h"

#include "Engine/World.h"
#include "LogOpenUnrealUtilities.h"
#include "TimerManager.h"

bool UExclusiveLock::TryLock(UObject* Key)
{
	if (IsValid(Key))
	{
		if (!IsLocked())
		{
			ActiveKey = Key;
			OnLockStateChanged.Broadcast(this, true);
			return true;
		}
		else if (Key == ActiveKey)
		{
			return true;
		}
	}

	return false;
}

bool UExclusiveLock::TryLockForDuration(UObject* Key, float Duration)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("TryLockForDuration cannot be used if the lock does not have a valid world context"));
		return false;
	}

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(
		Handle,
		[this, Key]() { TryUnlock(Key); },
		Duration,
		false);
	return TryLock(Key);
}

bool UExclusiveLock::TryUnlock(UObject* Key)
{
	if (IsLocked() && Key == ActiveKey)
	{
		ActiveKey = nullptr;
		OnLockStateChanged.Broadcast(this, false);
		return true;
	}

	return ActiveKey == nullptr;
}

bool UExclusiveLock::IsLocked() const
{
	return ActiveKey.IsValid() && !ActiveKey.IsStale();
}
