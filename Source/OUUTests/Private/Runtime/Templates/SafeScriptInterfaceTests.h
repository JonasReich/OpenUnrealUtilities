// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Templates/SafeScriptInterface.h"
#include "SafeScriptInterfaceTests.generated.h"




UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class USafeScriptTestInterface : public UInterface 
{
public:
	GENERATED_BODY()
};

class ISafeScriptTestInterface
{
public:
	GENERATED_BODY()

	virtual float ReturnFortyTwo() const = 0;
};


UCLASS(BlueprintType, Blueprintable)
class USafeScriptInterfaceTestObject : public UObject
{
	GENERATED_BODY()
public:
	// This is creepy shit D:
	template<typename T> using TScriptInterface = TSafeScriptInterface<T>;

	UPROPERTY()
	TScriptInterface<ISafeScriptTestInterface> TestInterface;
};

UCLASS(BlueprintType, Blueprintable)
class USafeScriptInterfaceTestObjectTwo : public UObject, public ISafeScriptTestInterface
{
	GENERATED_BODY()
public:

	virtual float ReturnFortyTwo() const override { return 42.f; }
};
