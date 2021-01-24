// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/**
 * Call a function on a blueprint implementable interface object.
 * The object can be any of [UObject*, TScriptInterface, IInterface*].
 * Only safe after validation via CanCallInterface()!
 */
#define CALL_INTERFACE(InterfaceClass, Function, InterfaceObject, ...) \
InterfaceClass::Execute_ ## Function(GetInterfaceObject(InterfaceObject), ## __VA_ARGS__);

template<typename T>
using TIsIInterface_T = typename TEnableIf<TIsIInterface<T>::Value>::Type;

/** Get the underlying object from an interface so you can call Execute_* functions on it */
template<typename T, typename = TIsIInterface_T<T>>
UObject* GetInterfaceObject(T* InterfaceObject)
{
	return Cast<UObject>(InterfaceObject);
}

/**
 * Get the underlying object from an interface so you can call Execute_* functions on it.
 * This overload for UObject* is useful for templates (see CALL_INTERFACE macro above).
 */
FORCEINLINE UObject* GetInterfaceObject(UObject* InterfaceObject)
{
	return InterfaceObject;
}

/** Get the underlying object from an interface so you can call Execute_* functions on it */
template<typename T, typename = TIsIInterface_T<T>>
UObject* GetInterfaceObject(TScriptInterface<T> InterfaceObject)
{
	return InterfaceObject.GetObject();
}

/**
 * Validate an interface object based on it's underlying UObject, extending IsValid(UObject*) functionality.
 * Also checks if the object actually implements the target interface.
 * Supports the following types:
 * - UObject*
 * - IInterfaceType*
 * - TScriptInterface<T>
 */
template<typename T>
FORCEINLINE bool IsValidInterface(UObject* InterfaceObject)
{
	return IsValid(InterfaceObject) && InterfaceObject->GetClass()->ImplementsInterface(T::UClassType::StaticClass());
}

 /**
  * Validate a IInterface pointer's underlying UObject.
  */
template<typename T, typename = TIsIInterface_T<T>>
FORCEINLINE bool IsValidInterface(T* InterfaceObject)
{
	return IsValid(GetInterfaceObject(InterfaceObject));
}

/**
 * Validate a TScriptInterface's underlying UObject.
 * Also makes sure the interface pointer is set to a correct value.
 */
template<typename T, typename = TIsIInterface_T<T>>
FORCEINLINE bool IsValidInterface(TScriptInterface<T>& InterfaceObject)
{
	UObject* ObjectPoiner = InterfaceObject.GetObject();
	bool bResult = IsValidInterface<T>(ObjectPoiner);
	InterfaceObject.SetInterface(bResult ? Cast<T>(ObjectPoiner) : nullptr);
	return bResult;
}

/** Override to throw compile time error for using const TScriptInterface<T> or const TScriptInterface<T>& */
template<typename T, typename = typename TIsIInterface_T<T>>
FORCEINLINE bool IsValIsValidInterfaceid(const TScriptInterface<T>& InterfaceObject)
{
	static_assert(false, "You should not use const TScriptInterface<T>! Three reasons why:\n"
		"\t- No memory benefit over passing by value\n"
		"\t- const TScriptInterface<T> cannot be invalidated when it turns stale or corrected by IsValid() check\n"
		"\t- No const safeness for pointer target, because const TScriptInterface<T> still allows calling non-const functions");
	return false;
}
