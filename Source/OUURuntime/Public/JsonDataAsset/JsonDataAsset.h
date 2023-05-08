// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/SubclassOf.h"

#include "JsonDataAsset.generated.h"

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
		};                                                                                                             \
	};

class UJsonDataAsset;

UENUM(BlueprintType)
enum class EJsonDataAccessMode : uint8
{
	Read,
	Write
};

namespace OUU::Runtime::JsonData
{
	OUURUNTIME_API FString GetSourceRoot_ProjectRelative(EJsonDataAccessMode AccessMode);

	OUURUNTIME_API FString GetSourceRoot_Full(EJsonDataAccessMode AccessMode);

#if WITH_EDITOR
	// Mount point for source files. Not required at runtime, but for some content browser functionality.
	OUURUNTIME_API FString GetSourceMountPointRoot_Package();
	OUURUNTIME_API FString GetSourceMountPointRoot_DiskFull();
#endif

	// Mount point for generated packages.
	// Save into Save dir, so the packages are not versioned and can safely be deleted on engine startup.
	OUURUNTIME_API FString GetCacheMountPointRoot_Package();
	OUURUNTIME_API FString GetCacheMountPointRoot_DiskFull();

	OUURUNTIME_API bool PackageIsJsonData(const FString& PackagePath);

	OUURUNTIME_API FString PackageToDataRelative(const FString& PackagePath);

	OUURUNTIME_API FString PackageToSourceFull(const FString& PackagePath, EJsonDataAccessMode AccessMode);

	// Take a path that is relative to the project root and convert it into a package path.
	OUURUNTIME_API FString SourceFullToPackage(const FString& FullPath, EJsonDataAccessMode AccessMode);

	OUURUNTIME_API FString PackageToObjectName(const FString& Package);

	OUURUNTIME_API bool ShouldIgnoreInvalidExtensions();
} // namespace OUU::Runtime::JsonData

USTRUCT(BlueprintType)
struct OUURUNTIME_API FJsonDataAssetPath
{
	GENERATED_BODY()
public:
	friend class FJsonDataAssetPathCustomization;
	friend class UAssetValidator_JsonDataAssetReferences;

public:
	FJsonDataAssetPath() = default;
	explicit FJsonDataAssetPath(const UJsonDataAsset* Object) : Path(Object) {}

	FORCEINLINE static FJsonDataAssetPath FromPackagePath(const FString& PackagePath)
	{
		FJsonDataAssetPath Result;
		Result.SetPackagePath(PackagePath);
		return Result;
	}

	// Try to resolve the path in memory, do not load if not loaded yet.
	UJsonDataAsset* Get() const;

	// Try to resolve the path in memory, load asset if not found.
	UJsonDataAsset* Resolve() const;

	/**
	 * Find existing asset representation of the json file or load asset into memory.
	 * Reloads all data members from json files.
	 */
	UJsonDataAsset* Load() const;

	FORCEINLINE bool IsNull() const { return Path.IsNull(); }

	FORCEINLINE void SetPackagePath(FString InPackagePath) { Path = InPackagePath; }
	FORCEINLINE FString GetPackagePath() const { return Path.GetLongPackageName(); }

	FORCEINLINE friend uint32 GetTypeHash(const FJsonDataAssetPath& Other) { return GetTypeHash(Other.Path); }

	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	bool ExportTextItem(
		FString& ValueStr,
		FJsonDataAssetPath const& DefaultValue,
		UObject* Parent,
		int32 PortFlags,
		UObject* ExportRootScope) const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);
	bool NetSerialize(FArchive& Ar, UPackageMap* PackageMap, bool& OutSuccess);

	FORCEINLINE bool operator==(const FJsonDataAssetPath& _Other) const { return Path == _Other.Path; }
	FORCEINLINE bool operator!=(const FJsonDataAssetPath& _Other) const { return Path != _Other.Path; }

private:
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UJsonDataAsset> Path;
};

OUU_DECLARE_JSON_DATA_ASSET_PTR_TRAITS(FJsonDataAssetPath);

// Equivalent to a regular TSoftObjectPtr for json data assets. If you derive a struct from this, you can give it the
// JsonDataAssetClass meta tag to specify a default class filter.
// For example, an implementation of this struct might look like this:
//
//	USTRUCT(BlueprintType, Meta = (JsonDataAssetClass = "/Script/MyPlugin.MyClass"))
//	struct GRIMVENTORY_API FMyClassSoftPtr
//	#if CPP
//		: public TSoftJsonDataAssetPtr<UMyClass>
//	#else
//		: public FSoftJsonDataAssetPtr
//	#endif
//	{
//		GENERATED_BODY()
//
//	public:
//		using TSoftJsonDataAssetPtr::TSoftJsonDataAssetPtr;
//	};
//
//	OUU_DECLARE_JSON_DATA_ASSET_PTR_TRAITS(FMyClassSoftPtr);
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

	FORCEINLINE UJsonDataAsset* Get() const { return Path.Get(); }

	FORCEINLINE bool IsNull() const { return Path.IsNull(); }

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

	FORCEINLINE friend uint32 GetTypeHash(const FSoftJsonDataAssetPtr& Other) { return GetTypeHash(Other.Path); }

	FORCEINLINE operator bool() const { return Path.IsNull() == false; }
	FORCEINLINE bool operator==(const FSoftJsonDataAssetPtr& Other) const { return Path == Other.Path; }
	FORCEINLINE bool operator!=(const FSoftJsonDataAssetPtr& Other) const { return Path != Other.Path; }
	FORCEINLINE bool operator==(const UJsonDataAsset* Other) const { return Get() == Other; }
	FORCEINLINE bool operator!=(const UJsonDataAsset* Other) const { return Get() != Other; }
	FORCEINLINE bool operator==(TYPE_OF_NULLPTR) const { return Path.Get() == nullptr; }
	FORCEINLINE bool operator!=(TYPE_OF_NULLPTR) const { return Path.Get() != nullptr; }
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

	template <typename = void>
	FORCEINLINE T* Get() const
	{
		return Cast<T>(FSoftJsonDataAssetPtr::Get());
	}

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

// Same as FSoftJsonDataAssetPtr, but behaving like a hard reference.
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
			HardReference = Path.Resolve();
		}

		return HardReference;
	}

	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	bool ExportTextItem(
		FString& ValueStr,
		FJsonDataAssetPtr const& DefaultValue,
		UObject* Parent,
		int32 PortFlags,
		UObject* ExportRootScope) const;
	bool SerializeFromMismatchedTag(const FPropertyTag& Tag, FStructuredArchive::FSlot Slot);
	bool NetSerialize(FArchive& Ar, UPackageMap* PackageMap, bool& OutSuccess);

	FORCEINLINE friend uint32 GetTypeHash(const FJsonDataAssetPtr& Other) { return GetTypeHash(Other.Path); }

	FORCEINLINE operator bool() const { return Path.IsNull() == false; }
	FORCEINLINE bool operator==(const FJsonDataAssetPtr& Other) const { return Path == Other.Path; }
	FORCEINLINE bool operator!=(const FJsonDataAssetPtr& Other) const { return Path != Other.Path; }
	FORCEINLINE bool operator==(const UJsonDataAsset* Other) const { return Get() == Other; }
	FORCEINLINE bool operator!=(const UJsonDataAsset* Other) const { return Get() != Other; }
	FORCEINLINE bool operator==(TYPE_OF_NULLPTR) const { return Path.Get() == nullptr; }
	FORCEINLINE bool operator!=(TYPE_OF_NULLPTR) const { return Path.Get() != nullptr; }
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

UCLASS(BlueprintType)
class UJsonDataAssetSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	// - USubsystem
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	// --

	// If true, asset saves/moves will export to json
	static bool AutoExportJsonEnabled();

	// Import all .json into UJsonDataAssets.
	// This does not delete stale UJsonDataAssets that don't have a matching .json file anymore.
	// It does reload all property data of existing json assets, unless bOnlyMissing is true.
	UFUNCTION()
	void ImportAllAssets(bool bOnlyMissing);

private:
#if WITH_EDITOR
	void CleanupAssetCache();

	UFUNCTION()
	void HandlePackageDeleted(UPackage* Package);

	UFUNCTION()
	void ModifyCook(TArray<FString>& OutExtraPackagesToCook);
#endif

	bool bAutoExportJson = false;
};

UCLASS()
class OUURUNTIME_API UJsonDataAssetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	static UJsonDataAsset* LoadJsonDataAsset(FJsonDataAssetPath Path);

	// @returns if reloading was successful
	UFUNCTION(BlueprintCallable)
	static bool ReloadJsonDataAsset(UJsonDataAsset* DataAsset);

	// Get the object pointed to by the given FSoftJsonDataAssetPtr if it is currently loaded.
	UFUNCTION(BlueprintPure, Meta = (DeterminesOutputType = Class), Category = "OUU")
	static UJsonDataAsset* GetSoftJsonDataAssetPtr(const FSoftJsonDataAssetPtr& Ptr, TSubclassOf<UJsonDataAsset> Class);

	// Get the object pointed to by the given FSoftJsonDataAssetPtr (and load it if it isn't loaded yet)
	UFUNCTION(BlueprintPure, Meta = (DeterminesOutputType = Class, DisplayName = "Load Synchronous"), Category = "OUU")
	static UJsonDataAsset* LoadJsonDataAssetPtrSyncronous(
		const FSoftJsonDataAssetPtr& Ptr,
		TSubclassOf<UJsonDataAsset> Class);

	// Get the object pointed to by the given FJsonDataAssetPtr.
	UFUNCTION(BlueprintPure, Meta = (DeterminesOutputType = Class), Category = "OUU")
	static UJsonDataAsset* GetJsonDataAssetPtr(const FJsonDataAssetPtr& Ptr, TSubclassOf<UJsonDataAsset> Class);

	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToRawPtr (SoftJsonDataAssetPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "RawPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static UJsonDataAsset* Conv_SoftJsonDataAssetPtrToRawPtr(const FSoftJsonDataAssetPtr& InPtr);
	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToSoftJsonDataAssetPtr (RawPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "SoftJsonDataAssetPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static FSoftJsonDataAssetPtr Conv_RawPtrToSoftJsonDataAssetPtr(UJsonDataAsset* InPtr);

	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToRawPtr (JsonDataAssetPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "RawPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static UJsonDataAsset* Conv_JsonDataAssetPtrToRawPtr(const FJsonDataAssetPtr& InPtr);
	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "ToJsonDataAssetPtr (RawPtr)",
			 CompactNodeTitle = "->",
			 ScriptMethod = "JsonDataAssetPtr",
			 Keywords = "cast convert",
			 BlueprintAutocast),
		Category = "OUU|Conversions")
	static FJsonDataAssetPtr Conv_RawPtrToJsonDataAssetPtr(UJsonDataAsset* InPtr);

private:
	static UJsonDataAsset* LoadJsonDataAsset_Internal(FJsonDataAssetPath Path, UJsonDataAsset* ExistingDataAsset);
};

UCLASS(BlueprintType, Blueprintable)
class OUURUNTIME_API UJsonDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	bool IsInJsonDataContentRoot() const;

	// Does this object have a matching json file to save to/load from?
	UFUNCTION(BlueprintPure)
	bool IsFileBasedJsonAsset() const;

	// Import/export json objects
	bool ImportJson(TSharedPtr<FJsonObject> JsonObject, bool bCheckClassMatches = true);
	TSharedRef<FJsonObject> ExportJson() const;

	UFUNCTION(BlueprintCallable)
	FString GetJsonFilePathAbs(EJsonDataAccessMode AccessMode) const;

	UFUNCTION(BlueprintPure)
	FJsonDataAssetPath GetPath() const;

	UFUNCTION(BlueprintCallable)
	bool ImportJsonFile();

	UFUNCTION(BlueprintCallable)
	bool ExportJsonFile() const;

public:
	// - UObject interface
	bool Rename(const TCHAR* NewName = nullptr, UObject* NewOuter = nullptr, ERenameFlags Flags = REN_None) override;
	void PostRename(UObject* OldOuter, const FName OldName) override;

	void PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext) override;
	// Not called for newly created objects
	void PostLoad() override;
	void PostDuplicate(bool bDuplicateForPIE) override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) override;
#endif

private:
	bool bIsInPostLoad = false;
};
