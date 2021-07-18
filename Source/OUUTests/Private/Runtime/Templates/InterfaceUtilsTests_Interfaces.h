// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InterfaceUtilsTests_Interfaces.generated.h"

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
