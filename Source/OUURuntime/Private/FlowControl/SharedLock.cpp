// Copyright (c) 2023 Jonas Reich & Contributors

#include "FlowControl/SharedLock.h"

#include "Engine/World.h"
#include "LogOpenUnrealUtilities.h"
#include "TimerManager.h"

USharedLock::FScopedLock::FScopedLock(USharedLock& InLock, UObject* InKey) : Lock(InLock), Key(InKey)
{
	Lock.Lock(Key);
}

USharedLock::FScopedLock::FScopedLock(FScopedLock&& Other) noexcept : Lock(Other.Lock), Key(Other.Key)
{
	Other.Key = nullptr;
}

USharedLock::FScopedLock::~FScopedLock() noexcept
{
	// Key may have been invalidated by move
	if (Key)
	{
		Lock.TryUnlock(Key);
	}
}

void USharedLock::Lock(UObject* Key)
{
	const bool bWasLockedBefore = IsLocked();
	if (IsValid(Key))
	{
		ActiveKeys.AddUnique(Key);
	}
	if (!bWasLockedBefore && IsLocked())
	{
		OnLockStateChanged.Broadcast(this, true);
	}
}

USharedLock::FScopedLock USharedLock::ScopedLock(UObject* Key)
{
	return {*this, Key};
}

void USharedLock::LockForDuration(UObject* Key, float Duration)
{
	const UWorld* World = GetWorld();
	if (!IsValid(World))
	{
		UE_LOG(
			LogOpenUnrealUtilities,
			Warning,
			TEXT("LockForDuration cannot be used if the lock does not have a valid world context"));
		return;
	}

	FTimerHandle Handle;
	World->GetTimerManager().SetTimer(
		Handle,
		[this, Key]() { TryUnlock(Key); },
		Duration,
		false);

	Lock(Key);
}

bool USharedLock::TryUnlock(UObject* Key)
{
	const bool bWasLockedBefore = IsLocked();
	ActiveKeys.RemoveSwap(Key);
	const bool bIsUnlocked = !CheckIsLocked();
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
	ActiveKeys.RemoveAllSwap([](auto Key) { return !Key.IsValid() && Key.IsStale(); });
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
