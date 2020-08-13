// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "InterfaceUtilsTests.generated.h"

// -- C++ Interface --

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UInterfaceUtilsTests_CppInterface : public UInterface
{
	GENERATED_BODY()
public:
};

class IInterfaceUtilsTests_CppInterface
{
	GENERATED_BODY()
public:
	virtual void SetNumber(float f) = 0;
	virtual float GetNumber() const = 0;
};

UCLASS()
class UInterfaceUtilsTests_CppInterface_Impl : public UObject,
	public IInterfaceUtilsTests_CppInterface
{
	GENERATED_BODY()
public:
	float Number;

	virtual void SetNumber(float f) override
	{
		Number = f;
	}

	virtual float GetNumber() const
	{
		return Number;
	}
};

// -- Blueprint Interface --
// #TODO
