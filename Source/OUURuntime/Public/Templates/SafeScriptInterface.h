// Copyright (c) 2020 Jonas Reich

#pragma once

#include "CoreMinimal.h"

/**
 * Templated version of FScriptInterface, which provides accessors and operators for referencing the interface portion of a UObject that implements a native interface.
 */
template< class InterfaceType>
class TSafeScriptInterface : public TScriptInterface<InterfaceType>
{
public:
	//static_assert(sizeof(InterfaceType) == 0, "it's actually used. yay!");

	/**
	 * Default constructor
	 */
	TSafeScriptInterface() {}

	/**
	 * Construction from nullptr
	 */
	TSafeScriptInterface(TYPE_OF_NULLPTR) {}

	/**
	 * Standard constructor.
	 *
	 * @param	SourceObject	a pointer to a UObject that implements the InterfaceType native interface class.
	 */
	template <class UObjectType> TSafeScriptInterface(UObjectType* SourceObject)
	{
		(*this) = SourceObject;
	}
	/**
	 * Copy constructor
	 */
	TSafeScriptInterface(const TSafeScriptInterface& Other)
	{
		SetObject(Other.GetObject());
		SetInterface(Other.GetInterface());
	}

	void SetInterfaceFromObject()
	{
		InterfaceType* SourceInterface = Cast<InterfaceType>(GetObject());
		SetInterface(SourceInterface);
	}

};
