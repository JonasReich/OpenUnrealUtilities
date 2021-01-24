// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "FlowControl/SharedLock.h"
#include "FlowControl/ExclusiveLock.h"
#include "LockTestHelpers.generated.h"

UCLASS(meta = (Hidden, HideDropDown))
class ULockTestsHelper : public UObject
{
	GENERATED_BODY()
public:
	UObject* Lock = nullptr;
	bool bLockAlwaysSame = true;
	TArray<bool> LockStateHistory;
	int32 NumDelegateCalls = 0;

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
		NumDelegateCalls++;

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

template<class LockType>
struct FOUULockTestEnvironment
{
	LockType* Lock;
	ULockTestsHelper* Helper;
	UObject* Key0;
	UObject* Key1;

	FOUULockTestEnvironment(UObject* Outer = GetTransientPackage())
	{
		Lock = NewObject<LockType>(Outer);
		Helper = NewObject<ULockTestsHelper>();
		Key0 = NewObject<UOUUTestObject>();
		Key1 = NewObject<UOUUTestObject>();

		AssignLockDelegate(Lock);
	}

	void AssignLockDelegate(UExclusiveLock* InLock)
	{
		InLock->OnLockStateChanged.AddDynamic(Helper, &ULockTestsHelper::HandleExclusiveLockStateChanged);
	}

	void AssignLockDelegate(USharedLock* InLock)
	{
		InLock->OnLockStateChanged.AddDynamic(Helper, &ULockTestsHelper::HandleSharedLockStateChanged);
	}
};
