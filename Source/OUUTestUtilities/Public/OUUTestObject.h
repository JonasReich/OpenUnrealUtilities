// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "OUUTestObject.generated.h"

/** Used in place of plain UObject for tests, because UObject itself is abstract */
UCLASS(hidedropdown)
class OUUTESTUTILITIES_API UOUUTestObject : public UObject
{
	GENERATED_BODY()
};

/** Used in place of plain UUserWidget for tests, because UUserWidget itself is abstract */
UCLASS(hidedropdown)
class OUUTESTUTILITIES_API UOUUTestWidget : public UUserWidget
{
	GENERATED_BODY()
};
