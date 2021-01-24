// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Templates/InterfaceUtilsTests_Interfaces.h"
#include "InterfaceUtilsTests_Implementations.generated.h"

// ------------------------
// -- C++ only Interface --
// ------------------------

UCLASS(meta = (Hidden, HideDropDown))
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

// -------------------------
// -- Blueprint Interface --
// -------------------------

// C++ implementation of IInterfaceUtilsTests_BpInterface
UCLASS(meta = (Hidden, HideDropDown))
class UInterfaceUtilsTests_BpInterface_CppImpl : public UObject,
	public IInterfaceUtilsTests_BpInterface
{
	GENERATED_BODY()
public:
	float Number;

	virtual void SetNumber_Implementation(float f) override
	{
		Number = f;
	}

	virtual float GetNumber_Implementation() const
	{
		return Number;
	}
};
