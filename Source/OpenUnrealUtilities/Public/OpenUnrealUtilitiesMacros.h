// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/**
 * Helper macro for creating a dynamic delegate, binding a function to it and returning the delegate.
 * Similar to BindDynamic()/AddDynamic() utility macros.
 */
#define CreateDynamic(DynamicDelegateType, UserObject, FuncName) \
[&UserObject]() -> DynamicDelegateType {\
DynamicDelegateType Delegate; \
Delegate.BindDynamic(Widget, FuncName); \
return Delegate; \
}()
