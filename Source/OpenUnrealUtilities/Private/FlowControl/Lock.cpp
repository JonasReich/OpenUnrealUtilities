// Copyright (c) 2020 Jonas Reich

#include "FlowControl/Lock.h"
#include "OpenUnrealUtilities.h"

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

	return false;
}

bool UExclusiveLock::IsLocked() const
{
	return ActiveKey.IsValid() && !ActiveKey.IsStale();
}

void USharedLock::Lock(UObject* Key)
{
	bool bWasLockedBefore = IsLocked();
	if (IsValid(Key))
	{
		ActiveKeys.AddUnique(Key);
	}
	if (!bWasLockedBefore && IsLocked())
	{
		OnLockStateChanged.Broadcast(this, true);
	}
}

void USharedLock::LockForDuration(UObject* Key, float Duration)
{
	UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("LockForDuration cannot be used if the lock does not have a valid world context"));
		return;
	}

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(Handle, [this, Key]() { TryUnlock(Key); }, Duration, false);

	Lock(Key);
}

bool USharedLock::TryUnlock(UObject* Key)
{
	bool bWasLockedBefore = IsLocked();
	ActiveKeys.RemoveSwap(Key);
	bool bIsUnlocked = !CheckIsLocked();
	if (bWasLockedBefore && bIsUnlocked)
	{
		OnLockStateChanged.Broadcast(this, false);
	}
	return bIsUnlocked;
}

bool USharedLock::IsLocked() const
{
	return ActiveKeys.Num() > 0;
}

bool USharedLock::CheckIsLocked()
{
	ActiveKeys.RemoveAllSwap([](auto Key) {
		return !Key.IsValid() && Key.IsStale();
	});
	return IsLocked();
}

TArray<UObject*> USharedLock::GetActiveKeys() const
{
	TArray<UObject*> Result;
	for (auto Key : ActiveKeys)
	{
		UObject* KeyObj = Key.Get();
		if (IsValid(KeyObj))
		{
			Result.Add(KeyObj);
		}
	}
	return Result;
}
