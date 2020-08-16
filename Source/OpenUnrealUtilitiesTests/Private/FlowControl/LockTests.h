// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "FlowControl/Lock.h"
#include "LockTests.generated.h"

UCLASS()
class ULockTestsHelper : public UObject
{
	GENERATED_BODY()
public:
	bool bDelegateWasCalled = false;
	UObject* Lock = nullptr;
	bool bLockAlwaysSame = true;
	TArray<bool> LockStateHistory;

	UFUNCTION()
	void HandleExclusiveLockStateChanged(UExclusiveLock* InLock, bool bInIsLocked)
	{
		HandleLockStateChanged_SharedImplementation(InLock, bInIsLocked);		
	}

	UFUNCTION()
	void HandleSharedLockStateChanged(USharedLock* InLock, bool bInIsLocked)
	{
		HandleLockStateChanged_SharedImplementation(InLock, bInIsLocked);
	}

	void HandleLockStateChanged_SharedImplementation(UObject* InLock, bool bInIsLocked)
	{
		bDelegateWasCalled = true;

		if (Lock == nullptr)
		{
			Lock = InLock;
		}
		else if (Lock != InLock)
		{
			bLockAlwaysSame = false;
		}

		LockStateHistory.Add(bInIsLocked);
	}
};
