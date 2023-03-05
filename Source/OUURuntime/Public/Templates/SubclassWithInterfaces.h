// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Traits/ConditionalType.h"

namespace OUU::Runtime::Private::SubclassWithInterface
{
	// Get a "plain" object pointer from its storage.
	// Only implemented for the types we support in TSubclassWithInterfaces
	template <class ObjectBaseClass>
	ObjectBaseClass* GetObjectPtr(ObjectBaseClass* InPtr)
	{
		return InPtr;
	}
	template <class ObjectBaseClass>
	ObjectBaseClass* GetObjectPtr(ObjectBaseClass& InRef)
	{
		return &InRef;
	}
	template <class ObjectBaseClass>
	ObjectBaseClass* GetObjectPtr(TSoftObjectPtr<ObjectBaseClass>& SoftObjectPtr)
	{
		return SoftObjectPtr.Get();
	}
	template <class ObjectBaseClass>
	ObjectBaseClass* GetObjectPtr(TObjectPtr<ObjectBaseClass>& SoftObjectPtr)
	{
		return SoftObjectPtr.Get();
	}
	template <class ObjectBaseClass>
	ObjectBaseClass* GetObjectPtr(TSubclassOf<ObjectBaseClass>& SoftObjectPtr)
	{
		return SoftObjectPtr.Get();
	}

	// Deduce the "plain" type of an object from its storage (e.g. TObject<TObjectPtr<U>>::Type -> U)
	// Only implemented for the types we support in TSubclassWithInterfaces
	template <typename StorageType>
	struct TObject
	{
		using Type = void;
		static_assert(
			sizeof(StorageType) > 0,
			"This storage type is not supported. Please use one of the following: "
			"T&, T*, TSoftObjectPtr<T>, TObjectPtr<T>, TSubclassOf<T>");
	};

#define SPECIALIZE_OBJECT_STORAGE(StorageType)                                                                         \
	template <typename ObjectType>                                                                                     \
	struct TObject<StorageType>                                                                                        \
	{                                                                                                                  \
		using Type = ObjectType;                                                                                       \
	};

	SPECIALIZE_OBJECT_STORAGE(ObjectType&)
	SPECIALIZE_OBJECT_STORAGE(ObjectType*)
	SPECIALIZE_OBJECT_STORAGE(TSoftObjectPtr<ObjectType>)
	SPECIALIZE_OBJECT_STORAGE(TObjectPtr<ObjectType>)
	SPECIALIZE_OBJECT_STORAGE(TSubclassOf<ObjectType>)

#undef SPECIALIZE_OBJECT_STORAGE

	template <typename StorageType>
	using TObjectT = typename TObject<StorageType>::Type;

} // namespace OUU::Runtime::Private::SubclassWithInterface

/**
 * Similar to TSubclassOf in that an object pointer can of a specified subclass can be stored.
 * In addition to this, you specify intefaces that ALL need to be implemented by the object.
 * This is checked during compile time (by constructors / conversion functions).
 *
 * There is a reference and pointer version.
 *
 * Has different GC properties based on the storage type.
 */
template <typename InStorageType, class InObjectBaseClass, class... InterfaceClasses>
struct TSubclassWithInterfaces_Base
{
public:
	using StorageType = InStorageType;
	using ObjectBaseClass = InObjectBaseClass;

	// Easiest check: Assume only InObjectBaseClass& is a reference and everything else are pointers.
	// This needs to be refined if we ever support other non-nullable types.
	const static bool bIsPointer = TIsSame<InStorageType, InObjectBaseClass*>::Value;

private:
	StorageType Object;

	ObjectBaseClass* GetObjectPtr() const { return OUU::Runtime::Private::SubclassWithInterface::GetObjectPtr(Object); }

	// Get an interface POINTER.
	// The public GetInterface<T>() member functions return T* or T& depending on bInIsPointer value.
	template <class InterfaceClass>
	InterfaceClass* GetInterfacePtr() const
	{
		static_assert(
			TOr<TIsSame<InterfaceClass, InterfaceClasses>...>::Value,
			"InterfaceClass is not part of TSubclassWithInterfaces InterfaceClasses");
		return Cast<InterfaceClass>(GetObjectPtr());
	}

public:
	TSubclassWithInterfaces_Base() = delete;

	/**
	 * The DerivedClass can be any class that is derived from ObjectBaseClass and from all InterfaceClasses.
	 */
	template <
		typename DerivedClass,
		typename = typename TEnableIf<TAnd<
			TIsDerivedFrom<DerivedClass, ObjectBaseClass>,
			TIsDerivedFrom<DerivedClass, InterfaceClasses>...>::Value>::Type>
	TSubclassWithInterfaces_Base(DerivedClass* InObject) : Object(InObject)
	{
	}

	template <
		typename DerivedClass,
		typename = typename TEnableIf<TAnd<
			TIsDerivedFrom<DerivedClass, ObjectBaseClass>,
			TIsDerivedFrom<DerivedClass, InterfaceClasses>...>::Value>::Type>
	TSubclassWithInterfaces_Base(DerivedClass& InObject) : Object(InObject)
	{
	}

	// Get the object as ObjectBaseClass* or ObjectBaseClass& depending on bIsPointer value
	StorageType GetObject() const { return Object; }

	// Implicit conversion to ObjectBaseClass* or ObjectBaseClass& depending on bIsPointer value
	operator StorageType() const { return GetObject(); }

	// Get a reference to the stored object cast to InterfaceClass*
	// Only possible with subclass structs that have bIsPointer = true.
	template <class InterfaceClass>
	typename TEnableIf<bIsPointer == true, InterfaceClass*>::Type GetInterface() const
	{
		return GetInterfacePtr<InterfaceClass>();
	}

	// Get a reference to the stored object cast to InterfaceClass&
	// Only possible with subclass structs that have bIsPointer = false.
	template <class InterfaceClass>
	typename TEnableIf<bIsPointer == false, InterfaceClass&>::Type GetInterface() const
	{
		return *GetInterfacePtr<InterfaceClass>();
	}

	// Overloads for equality comparison and type hash are required for container storage (e.g. in TSet)
	friend uint32 GetTypeHash(const TSubclassWithInterfaces_Base& Self) { return GetTypeHash(Self.GetObjectPtr()); }
	friend bool operator==(const TSubclassWithInterfaces_Base& LHS, const TSubclassWithInterfaces_Base& RHS)
	{
		return LHS.GetObjectPtr() == RHS.GetObjectPtr();
	}
};

template <class InObjectBaseClassWithStorageSpecifier, class... InInterfaceClasses>
using TSubclassWithInterfaces = TSubclassWithInterfaces_Base<
	InObjectBaseClassWithStorageSpecifier,
	typename OUU::Runtime::Private::SubclassWithInterface::TObject<InObjectBaseClassWithStorageSpecifier>::Type,
	InInterfaceClasses...>;
