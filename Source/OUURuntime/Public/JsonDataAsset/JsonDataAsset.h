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

USTRUCT(BlueprintType)
struct OUURUNTIME_API FJsonDataAssetPath
{
	GENERATED_BODY()
public:
	friend class FJsonDataAssetPathCustomization;

public:
	FJsonDataAssetPath() = default;
	explicit FJsonDataAssetPath(UJsonDataAsset* Object) : Path(Object) {}

	FORCEINLINE static FJsonDataAssetPath FromPackagePath(const FString& PackagePath)
	{
		FJsonDataAssetPath Result;
		Result.SetPackagePath(PackagePath);
		return Result;
	}

	UJsonDataAsset* Load();

	FORCEINLINE bool IsNull() const { return Path.IsNull(); }

	FORCEINLINE void SetPackagePath(FString InPackagePath) { Path = InPackagePath; }
	FORCEINLINE FString GetPackagePath() { return Path.GetLongPackageName(); }

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

private:
	void ImportAllAssetsDuringStartup();

	UFUNCTION()
	void HandlePackageDeleted(UPackage* Package);

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
	static UJsonDataAsset* LoadJsonDataAsset_Internal(FJsonDataAssetPath Path, UJsonDataAsset* const ExistingDataAsset);
};

UCLASS(BlueprintType, Blueprintable)
class OUURUNTIME_API UJsonDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure)
	bool IsInJsonDataContentRoot() const;

	// Import/export json objects
	bool ImportJson(TSharedPtr<FJsonObject> JsonObject, bool bCheckClassMatches = true);
	TSharedRef<FJsonObject> ExportJson() const;

	// Import/export to files on-disk
	UFUNCTION(BlueprintCallable)
	FString GetJsonFilePathAbs() const;

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
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context);
#endif

private:
	bool bIsInPostLoad = false;
};

UCLASS(BlueprintType)
class UTestJsonDataAsset2 : public UJsonDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString StringProperty;
};

UCLASS(BlueprintType)
class UTestJsonDataAsset : public UJsonDataAsset
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowedClasses = "TestJsonDataAsset2"))
	FJsonDataAssetPath LinkedJsonAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UObject> LinkedRegularAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString StringProperty;
};