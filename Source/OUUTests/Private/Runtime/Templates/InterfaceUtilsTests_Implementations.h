// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Runtime/Templates/InterfaceUtilsTests_Interfaces.h"

#include "InterfaceUtilsTests_Implementations.generated.h"

// ------------------------
// -- C++ only Interface --
// ------------------------

UCLASS(meta = (Hidden, HideDropDown))
class UInterfaceUtilsTests_CppInterface_Impl : public UObject, public IInterfaceUtilsTests_CppInterface
{
	GENERATED_BODY()
public:
	float Number;

	void SetNumber(float f) override { Number = f; }
	float GetNumber() const override { return Number; }
};

// -------------------------
// -- Blueprint Interface --
// -------------------------

// C++ implementation of IInterfaceUtilsTests_BpInterface
UCLASS(meta = (Hidden, HideDropDown))
class UInterfaceUtilsTests_BpInterface_CppImpl : public UObject, public IInterfaceUtilsTests_BpInterface
{
	GENERATED_BODY()
public:
	float Number;

	void SetNumber_Implementation(float f) override { Number = f; }
	float GetNumber_Implementation() const override { return Number; }
};
