// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "OUUTestObject.generated.h"

/** Used in place of plain UObject for tests, because UObject itself is abstract */
UCLASS(hidedropdown)
class OPENUNREALUTILITIESTESTS_API UOUUTestObject : public UObject
{
	GENERATED_BODY()
};
