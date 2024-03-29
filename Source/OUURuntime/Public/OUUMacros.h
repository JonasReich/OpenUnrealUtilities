// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Helper macro for creating a dynamic delegate, binding a function to it and returning the delegate.
 * Similar to BindDynamic()/AddDynamic() utility macros.
 */
#define CreateDynamic(DynamicDelegateType, UserObject, FuncName)                                                       \
	[&]() -> DynamicDelegateType {                                                                                     \
		DynamicDelegateType Delegate;                                                                                  \
		Delegate.BindDynamic(UserObject, FuncName);                                                                    \
		return Delegate;                                                                                               \
	}()
