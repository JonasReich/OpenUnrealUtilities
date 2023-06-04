// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Templates/SubclassOf.h"

#include "JsonDataAssetPath.generated.h"

class UJsonDataAsset;

/**
 * Paths to json data assets.
 * This is the primary means by which references to json data content are serialized and resolved.
 * The API is meant to be as close as possible to FSoftObjectPath as is sensible.
 *
 * There are also pointer types based on this path type declarede in JsonDataAsset/JsonDataAssetPointers.h which
 * implement an API that is closer to regular asset pointers, but most of the internal code of this module (e.g. editor
 * extensions for json content) uses this type.
 *
 * Json data assets use a path syntax equivalent to package paths to the outermost of generated assets.
 * The json data assets do not support multiple subobjects and packages are only used in editor context as they are
 * not loaded via the regular asset loader at runtime, so all paths to json objects use the more simplified syntax
 * of package paths (e.g. '/JsonData/Folder/Asset') instead of object paths (e.g. '/JsonData/Folder/Asset.Asset')
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FJsonDataAssetPath
{
	GENERATED_BODY()
public:
	friend class FJsonDataAssetPathCustomization;
	friend class UAssetValidator_JsonDataAssetReferences;
	friend class UJsonDataAssetSubsystem;

public:
	FJsonDataAssetPath() = default;
	explicit FJsonDataAssetPath(const UJsonDataAsset* Object);

	FORCEINLINE static FJsonDataAssetPath FromPackagePath(const FString& PackagePath)
	{
		FJsonDataAssetPath Result;
		Result.SetPackagePath(PackagePath);
		return Result;
	}

	/** Try to resolve the path in memory, do NOT load if not found. */
	UJsonDataAsset* ResolveObject() const;

	// DEPRECATED Try to resolve the path in memory, do not load if not loaded yet.
	UE_DEPRECATED(
		5.1,
		"Changed signatures to better mirror FSoftObjectPath API and avoid misuse. Use ResolveObject() instead of "
		"Get().")
	FORCEINLINE UJsonDataAsset* Get() const { return ResolveObject(); }

	/** Try to resolve the path in memory, LOAD asset if not found. */
	UJsonDataAsset* LoadSynchronous() const;

	// DEPRECATED Try to resolve the path in memory, load asset if not loaded yet.
	UE_DEPRECATED(
		5.1,
		"Changed signatures to better mirror FSoftObjectPath API and avoid misuse. Use LoadSynchronous() instead of "
		"Resolve(). Use ResolveObject() if you want to retrieve in memory and never want to cause a synchronous load.")
	FORCEINLINE UJsonDataAsset* Resolve() const
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return LoadSynchronous();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	/** Try to resolve the path in memory, LOAD asset if not found. ALWAYS reload members from json source. */
	UJsonDataAsset* ForceReload() const;

	// DEPRECATED Try to resolve the path in memory and reload all properties, newly load asset if not found.
	UE_DEPRECATED(
		5.1,
		"Changed signatures to better mirror FSoftObjectPath API and avoid misuse. Use ForceReload() instead of "
		"Load(). Use LoadSynchronous() if you don't want to force reloading.")
	FORCEINLINE UJsonDataAsset* Load() const
	{
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		return ForceReload();
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
	}

	FORCEINLINE bool IsNull() const { return Path.IsNull(); }

	/** Reset the path to null */
	void Reset() { Path.Reset(); }

	/**
	 * Set from a long package name string (e.g. "/JsonData/Folder/Asset").
	 * Does not work with other string formats (e.g. object paths).
	 */
	void SetPackagePath(const FString& InPackagePath);

	/**
	 * Set from an object path (e.g. "/JsonData/Folder/Asset.Asset" or "ClassName '/JsonData/Folder/Asset.Asset'").
	 * Does not work with package paths.
	 */
	void SetObjectPath(const FString& InObjectPath);

	/**
	 * Set from a string.
	 * Supports multiple formats including package paths and object paths.
	 */
	void SetFromString(const FString& InString);

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
	bool Serialize(FArchive& Ar);
	bool Serialize(FStructuredArchive::FSlot Slot);

	FORCEINLINE bool operator==(const FJsonDataAssetPath& _Other) const { return Path == _Other.Path; }
	FORCEINLINE bool operator!=(const FJsonDataAssetPath& _Other) const { return Path != _Other.Path; }

private:
	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UJsonDataAsset> Path;
};

template <>
struct TStructOpsTypeTraits<FJsonDataAssetPath> : public TStructOpsTypeTraitsBase2<FJsonDataAssetPath>
{
	enum
	{
		WithExportTextItem = true,
		WithImportTextItem = true,
		WithStructuredSerializeFromMismatchedTag = true,
		WithNetSerializer = true,
		WithSerializer = true,
		WithStructuredSerializer = true,
	};
};
