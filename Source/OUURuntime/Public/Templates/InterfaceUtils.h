// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Templates/Casts.h"
#include "UObject/ScriptInterface.h"

/**
 * Call a function on a blueprint implementable interface object.
 * The object can be any of [UObject*, TScriptInterface, IInterface*].
 * Only safe after validation of the InterfaceObject, e.g. via IsValidInterface()!
 * @param	InterfaceType		The IInterface type
 * @param	Function			The function member of the Interface you want to invoke
 * @param	InterfaceObject		An object pointer, interface pointer or script interface of the matching interface
 */
#define CALL_INTERFACE(InterfaceType, Function, InterfaceObject, ...)                                                  \
	((OUU::Runtime::Private::Interface::Ignore(&InterfaceType::Function)),                                             \
	 (InterfaceType::Execute_##Function(GetInterfaceObject(InterfaceObject), ##__VA_ARGS__)))

/**
 * Try to call a function on a blueprint implementable interface object if it is valid.
 * The object can be any of [UObject*, TScriptInterface, IInterface*].
 */
#define TRY_CALL_INTERFACE(InterfaceType, Function, InterfaceObject, ...)                                              \
	if (IsValidInterface<InterfaceType>(InterfaceObject))                                                              \
	{                                                                                                                  \
		CALL_INTERFACE(InterfaceType, Function, InterfaceObject, ##__VA_ARGS__);                                       \
	}

/**
 * Try to assign a variable from a function on a blueprint implementable interface object if it is valid.
 * The object can be any of [UObject*, TScriptInterface, IInterface*].
 * @param AssignTarget The property that will be assigned if the function is called.
 */
#define TRY_ASSIGN_INTERFACE(AssignTarget, InterfaceType, Function, InterfaceObject, ...)                              \
	if (IsValidInterface<InterfaceType>(InterfaceObject))                                                              \
	{                                                                                                                  \
		AssignTarget = CALL_INTERFACE(InterfaceType, Function, InterfaceObject, ##__VA_ARGS__);                        \
	}

/**
 * Assign a variable from a function on a blueprint implementable interface object if it is valid.
 * If it is invalid, assign the fallback value.
 * The object can be any of [UObject*, TScriptInterface, IInterface*].
 * @param AssignTarget The property that will be assigned if the function is called.
 * @param FallbackValue The fallback value that will be used if InterfaceObject is invalid.
 */
#define ASSIGN_INTERFACE(AssignTarget, FallbackValue, InterfaceType, Function, InterfaceObject, ...)                   \
	TRY_ASSIGN_INTERFACE(AssignTarget, InterfaceType, Function, InterfaceObject, ##__VA_ARGS__)                        \
	else { AssignTarget = FallbackValue; }

template <typename T>
using TIsIInterface_T = typename TEnableIf<TIsIInterface<T>::Value>::Type;

/** Get the underlying object from an interface so you can call Execute_* functions on it */
template <typename T, typename = TIsIInterface_T<T>>
UObject* GetInterfaceObject(T* InterfaceObject)
{
	return Cast<UObject>(InterfaceObject);
}

/**
 * Get the underlying object from an interface so you can call Execute_* functions on it.
 * This overload for non-const UObject* is useful for templates (see CALL_INTERFACE macro above).
 */
FORCEINLINE UObject* GetInterfaceObject(UObject* InterfaceObject)
{
	return InterfaceObject;
}

/**
 * Get the underlying object from an interface so you can call Execute_* functions on it.
 * This overload for const UObject* is useful for templates (see CALL_INTERFACE macro above).
 */
FORCEINLINE const UObject* GetInterfaceObject(const UObject* InterfaceObject)
{
	return InterfaceObject;
}

/** Get the underlying object from an interface so you can call Execute_* functions on it */
template <typename T, typename = TIsIInterface_T<T>>
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
template <typename T>
FORCEINLINE bool IsValidInterface(const UObject* InterfaceObject)
{
	return IsValid(InterfaceObject) && InterfaceObject->GetClass()->ImplementsInterface(T::UClassType::StaticClass());
}

/**
 * Validate a IInterface pointer's underlying UObject.
 */
template <typename T, typename = TIsIInterface_T<T>>
FORCEINLINE bool IsValidInterface(T* InterfaceObject)
{
	return IsValid(GetInterfaceObject(InterfaceObject));
}

/**
 * Validate a TScriptInterface's underlying UObject.
 * Also makes sure the interface pointer is set to a correct value.
 */
template <typename T, typename = TIsIInterface_T<T>>
FORCEINLINE bool IsValidInterface(TScriptInterface<T>& InterfaceObject)
{
	UObject* ObjectPointer = InterfaceObject.GetObject();
	bool bResult = IsValidInterface<T>(ObjectPointer);
	InterfaceObject.SetInterface(bResult ? Cast<T>(ObjectPointer) : nullptr);
	return bResult;
}

/** Override to throw compile time error for using const TScriptInterface<T> or const TScriptInterface<T>& */
template <typename T, typename = TIsIInterface_T<T>>
FORCEINLINE bool IsValidInterface(const TScriptInterface<T>& InterfaceObject)
{
	static_assert(
		sizeof(T) == -1,
		"You should not use const TScriptInterface<T>! Three reasons why:\n"
		"\t- No memory benefit over passing by value\n"
		"\t- const TScriptInterface<T> cannot be invalidated when it turns stale or corrected by IsValid() check\n"
		"\t- No const safeness for pointer target, because const TScriptInterface<T> still allows calling non-const "
		"functions");
	return false;
}

namespace OUU::Runtime::Private::Interface
{
	/* Use this to let the compiler ignore what you have passed in */
	template <typename... Ts>
	void Ignore(Ts...)
	{
	}
} // namespace OUU::Runtime::Private::Interface
