// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "InterfaceUtilsTests.generated.h"

// ------------------------
// -- C++ only Interface --
// ------------------------

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

// -------------------------
// -- Blueprint Interface --
// -------------------------

UINTERFACE(BlueprintType)
class UInterfaceUtilsTests_BpInterface : public UInterface
{
	GENERATED_BODY()
public:
};

class IInterfaceUtilsTests_BpInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent)
	void SetNumber(float f);
	virtual void SetNumber_Implementation(float f) PURE_VIRTUAL(SetNumber_Implementation)

	UFUNCTION(BlueprintNativeEvent)
	float GetNumber() const;
	virtual float GetNumber_Implementation() const PURE_VIRTUAL(GetNumber_Implementation, return 0.f;)
};

// C++ implementation of IInterfaceUtilsTests_BpInterface
UCLASS()
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
