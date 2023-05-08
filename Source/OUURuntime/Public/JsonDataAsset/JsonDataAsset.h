// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Subsystems/EngineSubsystem.h"
#include "Templates/SubclassOf.h"

#include "JsonDataAsset.generated.h"

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
	explicit FJsonDataAssetPath(UJsonDataAsset* Object) : Path(Object) {}

	FORCEINLINE static FJsonDataAssetPath FromPackagePath(const FString& PackagePath)
	{
		FJsonDataAssetPath Result;
		Result.SetPackagePath(PackagePath);
		return Result;
	}

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

	bool ImportTextItem(const TCHAR*& Buffer, int32 PortFlags, UObject* Parent, FOutputDevice* ErrorText);
	bool ExportTextItem(
		FString& ValueStr,
		FJsonDataAssetPath const& DefaultValue,
		UObject* Parent,
		int32 PortFlags,
		UObject* ExportRootScope) const;

private:
	FORCEINLINE TSoftObjectPtr<UJsonDataAsset> Get() { return Path; }

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
	};
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
	UFUNCTION()
	void HandlePackageDeleted(UPackage* Package);

	UFUNCTION()
	void ModifyCook(TArray<FString>& OutExtraPackagesToCook);

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
