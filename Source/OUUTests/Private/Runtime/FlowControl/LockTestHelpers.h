// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "AutomationTestWorld.h"
#include "FlowControl/ExclusiveLock.h"
#include "FlowControl/SharedLock.h"
#include "OUUTestObject.h"

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

template <class LockType>
struct FOUULockTestEnvironment
{
	LockType* Lock = nullptr;
	ULockTestsHelper* Helper = nullptr;
	UObject* Key0 = nullptr;
	UObject* Key1 = nullptr;

	FOUUScopedAutomationTestWorld TestWorld{"FOUULockTestEnvironment"};

	FOUULockTestEnvironment(FAutomationTestBase& _OwningAutomationTest)
	{
		// Ignore errors that appeared during world creation
		_OwningAutomationTest.ClearExecutionInfo();

		Lock = NewObject<LockType>(TestWorld.World);
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
