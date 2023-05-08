// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "JsonDataAssetPath.h"

#include "JsonDataAssetPointers.generated.h"

class UJsonDataAsset;

//---------------------------------------------------------------------------------------------------------------------

#define OUU_DECLARE_JSON_DATA_ASSET_PTR_TRAITS(PtrTypeName)                                                            \
	template <>                                                                                                        \
	struct TStructOpsTypeTraits<PtrTypeName> : public TStructOpsTypeTraitsBase2<PtrTypeName>                           \
	{                                                                                                                  \
		enum                                                                                                           \
		{                                                                                                              \
			WithExportTextItem = true,                                                                                 \
			WithImportTextItem = true,                                                                                 \
			WithStructuredSerializeFromMismatchedTag = true,                                                           \
			WithNetSerializer = true,                                                                                  \
			WithSerializer = true,                                                                                     \
			WithStructuredSerializer = true,                                                                           \
		};                                                                                                             \
	};

//---------------------------------------------------------------------------------------------------------------------

/**
 * Equivalent of FSoftObjectPtr for json data assets.
 *
 * You can create derived structs that combine the serialization and loading properties of this type combined with
 * TSoftJsonDataAssetPtr which allows all members to return more specific typed results.
 * You'll also need to specify the 'JsonDataAssetClass' meta tag to get the desired behavior for details panels.
 *
 * For example, an implementation of this struct might look like this:
 *
 *		USTRUCT(BlueprintType, Meta = (JsonDataAssetClass = "/Script/MyModule.MyClass"))
 *		struct MYMODULE_API FMyClassSoftPtr
 *		#if CPP
 *			: public TSoftJsonDataAssetPtr<UMyClass>
 *		#else
 *			: public FSoftJsonDataAssetPtr
 *		#endif
 *		{
 *			GENERATED_BODY()
 *
 *		public:
 *			using TSoftJsonDataAssetPtr::TSoftJsonDataAssetPtr;
 *		};
 *
 *		OUU_DECLARE_JSON_DATA_ASSET_PTR_TRAITS(FMyClassSoftPtr);
 *
 * This would also allow using these references for Blueprints. To make them loadable in Blueprint graphs, you'll need
 * to supply additional conversion functions via a BlueprintFunctionLibrary.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FSoftJsonDataAssetPtr
{
	GENERATED_BODY()

public:
	friend class FJsonDataAssetPathCustomization;

public:
	FSoftJsonDataAssetPtr() = default;
	FSoftJsonDataAssetPtr(const FSoftJsonDataAssetPtr& Other) = default;
	FSoftJsonDataAssetPtr(FJsonDataAssetPath InPath);
	FSoftJsonDataAssetPtr(const UJsonDataAsset* Object);
	FSoftJsonDataAssetPtr(TYPE_OF_NULLPTR) {}

	/**
	 * Dereference the soft pointer.
	 * @return nullptr if this object is not in memory or the path was invalid, otherwise a valid UObject pointer.
	 */
	FORCEINLINE UJsonDataAsset* Get() const { return Path.ResolveObject(); }

	/**
	 * Test if this can never point to a live UObject
	 * @return true if this is explicitly pointing to no object
	 */
	FORCEINLINE bool IsNull() const { return Path.IsNull(); }

	FORCEINLINE const FJsonDataAssetPath& ToJsonDataAssetPath() const { return Path; }

	/** Synchronously load (if necessary) and return the object represented by this pointer */
	UJsonDataAsset* LoadSynchronous() const;

	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	bool ExportTextItem(
		FString& ValueStr,
		FSoftJsonDataAssetPtr const& DefaultValue,
		UObject* Parent,
		int32 PortFlags,
		UObject* ExportRootScope) const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);
	bool NetSerialize(FArchive& Ar, UPackageMap* PackageMap, bool& OutSuccess);
	bool Serialize(FArchive& Ar);
	bool Serialize(FStructuredArchive::FSlot Slot);

	FORCEINLINE friend uint32 GetTypeHash(const FSoftJsonDataAssetPtr& Other) { return GetTypeHash(Other.Path); }

	FORCEINLINE operator bool() const { return Get() != nullptr; }
	FORCEINLINE bool operator==(const FSoftJsonDataAssetPtr& Other) const { return Path == Other.Path; }
	FORCEINLINE bool operator!=(const FSoftJsonDataAssetPtr& Other) const { return Path != Other.Path; }
	FORCEINLINE bool operator==(const UJsonDataAsset* Other) const { return Get() == Other; }
	FORCEINLINE bool operator!=(const UJsonDataAsset* Other) const { return Get() != Other; }
	FORCEINLINE bool operator==(TYPE_OF_NULLPTR) const { return Path.ResolveObject() == nullptr; }
	FORCEINLINE bool operator!=(TYPE_OF_NULLPTR) const { return Path.ResolveObject() != nullptr; }
	FORCEINLINE UJsonDataAsset& operator*() const
	{
		const auto Object = Get();
		check(Object);
		return *Object;
	}
	FORCEINLINE UJsonDataAsset* operator->() const
	{
		const auto Object = Get();
		check(Object);
		return Object;
	}

	FORCEINLINE FSoftJsonDataAssetPtr& operator=(const FSoftJsonDataAssetPtr& Other)
	{
		Path = Other.Path;
		return *this;
	}
	FORCEINLINE FSoftJsonDataAssetPtr& operator=(FSoftJsonDataAssetPtr&& Other)
	{
		Path = MoveTemp(Other.Path);
		return *this;
	}

private:
	UPROPERTY(EditAnywhere)
	FJsonDataAssetPath Path;
};

OUU_DECLARE_JSON_DATA_ASSET_PTR_TRAITS(FSoftJsonDataAssetPtr);

template <typename T>
struct TSoftJsonDataAssetPtr : public FSoftJsonDataAssetPtr
{
public:
	TSoftJsonDataAssetPtr() = default;
	TSoftJsonDataAssetPtr(const TSoftJsonDataAssetPtr& Other) = default;
	TSoftJsonDataAssetPtr(FJsonDataAssetPath InPath) : FSoftJsonDataAssetPtr(InPath) {}
	TSoftJsonDataAssetPtr(TYPE_OF_NULLPTR) {}
	template <typename = void>
	TSoftJsonDataAssetPtr(T* Object) : FSoftJsonDataAssetPtr(Object)
	{
	}

	/**
	 * Dereference the soft pointer.
	 * @return nullptr if this object is not in memory or the path was invalid, otherwise a valid pointer.
	 */
	template <typename = void>
	FORCEINLINE T* Get() const
	{
		return Cast<T>(FSoftJsonDataAssetPtr::Get());
	}

	/** Synchronously load (if necessary) and return the object represented by this pointer */
	template <typename = void>
	T* LoadSynchronous() const
	{
		return Cast<T>(FSoftJsonDataAssetPtr::LoadSynchronous());
	}

	FORCEINLINE friend uint32 GetTypeHash(const TSoftJsonDataAssetPtr& Other)
	{
		return GetTypeHash(static_cast<const FSoftJsonDataAssetPtr&>(Other));
	}

	using FSoftJsonDataAssetPtr::operator==;
	using FSoftJsonDataAssetPtr::operator!=;
	template <typename = void>
	FORCEINLINE bool operator==(const T* Other) const
	{
		return Get() == Other;
	}
	template <typename = void>
	FORCEINLINE bool operator!=(const T* Other) const
	{
		return Get() != Other;
	}
	template <typename = void>
	FORCEINLINE T& operator*() const
	{
		const auto Object = Get();
		check(Object);
		return *Object;
	}
	template <typename = void>
	FORCEINLINE T* operator->() const
	{
		const auto Object = Get();
		check(Object);
		return Object;
	}
};

//---------------------------------------------------------------------------------------------------------------------

/**
 * Similar to FSoftJsonDataAssetPtr, but behaving closer to a hard reference.
 * Contrary to TObjectPtr or raw object pointers in UPROPERTYs, these pointers can't load assets implicitly.
 * Instead they will only lazily load them on first access. Because of this, it's reccommended to preload
 * large json assets (e.g. via your game's asset manager).
 *
 * This class also has a templated variant (TJsonDataAssetPtr) that can be used in derived structs similarly to
 * TSoftJsonDataAssetPtr.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FJsonDataAssetPtr
{
	GENERATED_BODY()

public:
	friend class FJsonDataAssetPathCustomization;

public:
	FJsonDataAssetPtr() = default;
	FJsonDataAssetPtr(const FJsonDataAssetPtr& Other) = default;
	FJsonDataAssetPtr(FJsonDataAssetPath InPath);
	FJsonDataAssetPtr(const UJsonDataAsset* Object);
	FJsonDataAssetPtr(TYPE_OF_NULLPTR) {}

	FORCEINLINE UJsonDataAsset* Get() const
	{
		if (HardReference == nullptr)
		{
			HardReference = Path.LoadSynchronous();
		}

		return HardReference;
	}

	FORCEINLINE const FJsonDataAssetPath& ToJsonDataAssetPath() const { return Path; }

#if WITH_EDITOR
	void NotifyPathChanged();
#endif

	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	bool ExportTextItem(
		FString& ValueStr,
		FJsonDataAssetPtr const& DefaultValue,
		UObject* Parent,
		int32 PortFlags,
		UObject* ExportRootScope) const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);
	bool NetSerialize(FArchive& Ar, UPackageMap* PackageMap, bool& OutSuccess);
	bool Serialize(FArchive& Ar);
	bool Serialize(FStructuredArchive::FSlot Slot);

	FORCEINLINE friend uint32 GetTypeHash(const FJsonDataAssetPtr& Other) { return GetTypeHash(Other.Path); }

	FORCEINLINE operator bool() const { return Get() != nullptr; }
	FORCEINLINE bool operator==(const FJsonDataAssetPtr& Other) const { return Path == Other.Path; }
	FORCEINLINE bool operator!=(const FJsonDataAssetPtr& Other) const { return Path != Other.Path; }
	FORCEINLINE bool operator==(const UJsonDataAsset* Other) const { return Get() == Other; }
	FORCEINLINE bool operator!=(const UJsonDataAsset* Other) const { return Get() != Other; }
	FORCEINLINE bool operator==(TYPE_OF_NULLPTR) const { return Get() == nullptr; }
	FORCEINLINE bool operator!=(TYPE_OF_NULLPTR) const { return Get() != nullptr; }
	FORCEINLINE UJsonDataAsset& operator*() const
	{
		const auto Object = Get();
		check(Object);
		return *Object;
	}
	FORCEINLINE UJsonDataAsset* operator->() const
	{
		const auto Object = Get();
		check(Object);
		return Object;
	}

	FORCEINLINE FJsonDataAssetPtr& operator=(const FJsonDataAssetPtr& Other)
	{
		Path = Other.Path;
		HardReference = Other.HardReference;
		return *this;
	}
	FORCEINLINE FJsonDataAssetPtr& operator=(FJsonDataAssetPtr&& Other)
	{
		Path = MoveTemp(Other.Path);
		HardReference = Other.HardReference;
		return *this;
	}

private:
	UPROPERTY(EditAnywhere)
	FJsonDataAssetPath Path;
	UPROPERTY(Transient)
	mutable UJsonDataAsset* HardReference = nullptr;
};

OUU_DECLARE_JSON_DATA_ASSET_PTR_TRAITS(FJsonDataAssetPtr);

template <typename T>
struct TJsonDataAssetPtr : public FJsonDataAssetPtr
{
public:
	TJsonDataAssetPtr() = default;
	TJsonDataAssetPtr(const TJsonDataAssetPtr& Other) = default;
	TJsonDataAssetPtr(FJsonDataAssetPath InPath) : FJsonDataAssetPtr(InPath) {}
	TJsonDataAssetPtr(TYPE_OF_NULLPTR) {}
	template <typename = void>
	TJsonDataAssetPtr(T* Object) : FJsonDataAssetPtr(Object)
	{
	}

	template <typename = void>
	FORCEINLINE T* Get() const
	{
		return Cast<T>(FJsonDataAssetPtr::Get());
	}

	FORCEINLINE friend uint32 GetTypeHash(const TJsonDataAssetPtr& Other)
	{
		return GetTypeHash(static_cast<const FJsonDataAssetPtr&>(Other));
	}

	using FJsonDataAssetPtr::operator==;
	using FJsonDataAssetPtr::operator!=;
	template <typename = void>
	FORCEINLINE bool operator==(const T* Other) const
	{
		return Get() == Other;
	}
	template <typename = void>
	FORCEINLINE bool operator!=(const T* Other) const
	{
		return Get() != Other;
	}
	template <typename = void>
	FORCEINLINE T& operator*() const
	{
		const auto Object = Get();
		check(Object);
		return *Object;
	}
	template <typename = void>
	FORCEINLINE T* operator->() const
	{
		const auto Object = Get();
		check(Object);
		return Object;
	}
	template <typename = void>
	operator T*() const
	{
		return Get();
	}
};
