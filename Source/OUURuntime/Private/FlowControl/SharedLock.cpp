// Copyright (c) 2021 Jonas Reich

#include "FlowControl/SharedLock.h"
#include "LogOpenUnrealUtilities.h"

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
