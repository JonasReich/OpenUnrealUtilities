// Copyright (c) 2021 Jonas Reich

#include "FlowControl/ExclusiveLock.h"
#include "LogOpenUnrealUtilities.h"

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
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("TryLockForDuration cannot be used if the lock does not have a valid world context"));
		return false;
	}

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, [this, Key]() { TryUnlock(Key); }, Duration, false);
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
