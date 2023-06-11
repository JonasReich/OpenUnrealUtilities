// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Dom/JsonObject.h"
#include "Engine/DataAsset.h"
#include "Misc/EngineVersion.h"

// Include all the utility headers from here for backwards compatibility
#include "JsonDataAsset/JsonDataAssetPath.h"
#include "JsonDataAsset/JsonDataAssetPointers.h"
#include "JsonDataAsset/JsonDataCustomVersions.h"

// Explicitly exclude the json data asset library. It's mainly intended for Blueprint use.
// #include "JsonDataAsset/JsonDataAssetLibrary.h"
// Explicitly exclude the json data asset subsystem. It's mainly intended for internal use.
// #include "JsonDataAsset/JsonDataAssetSubsystem.h"

#include "JsonDataAsset.generated.h"

class UJsonDataAsset;

UENUM(BlueprintType)
enum class EJsonDataAccessMode : uint8
{
	Read,
	Write
};

/**
 * Data assets derived from this type must be stored in the special /JsonData/ content root.
 * They are backed not by uasset binary files, but by json text files.
 *
 * The json files are preserved during cook and shipped as plain-text files, so they are usable as configuration
 * files for lightweight data that should be made accessible and changeable even after packaging - e.g. for
 * balancing purposes or modding support.
 */
UCLASS(BlueprintType, Blueprintable)
class OUURUNTIME_API UJsonDataAsset : public UDataAsset
{
	GENERATED_BODY()

	friend FJsonDataAssetPath;

	using FCustomVersionMap = TMap<FGuid, int32>;

public:
	/** @returns if the json asset is placed within the /JsonData/ root directory */
	UFUNCTION(BlueprintPure)
	bool IsInJsonDataContentRoot() const;

	/**
	 * @returns	if this object is an asset that can have a matching json file to save to / load from.
	 *			May be true for objects that do not have a json file on disk YET or ANYMORE that.
	 *			Will be false for objects that can NEVER have a disk file, like class default objects.
	 */
	UFUNCTION(BlueprintPure)
	bool IsFileBasedJsonAsset() const;

	/**
	 * Import json object into properties.
	 * This may fail if either the format of the json object is not correct or properties cannot be resolved.
	 * @param	bCheckClassMatches		If true, it's checked that the class metadata is an exact match to this
	 * class.
	 * @returns	if import was successful
	 */
	bool ImportJson(TSharedPtr<FJsonObject> JsonObject, bool bCheckClassMatches = true);

	/** Export json object without writing to disk */
	TSharedRef<FJsonObject> ExportJson() const;

	UFUNCTION(BlueprintCallable)
	FString GetJsonFilePathAbs(EJsonDataAccessMode AccessMode) const;

	UFUNCTION(BlueprintPure)
	FJsonDataAssetPath GetPath() const;

	/**
	 * Import the property values of this object from its json source file.
	 * @returns if loading was successful
	 */
	UFUNCTION(BlueprintCallable)
	bool ImportJsonFile();

	/**
	 * Save the property values of this object to its json source file.
	 * @returns if saving was successful
	 */
	UFUNCTION(BlueprintCallable)
	bool ExportJsonFile() const;

	/**
	 * Called after importing json data, can be used to fix up legacy data.
	 * @returns if loading was successful.
	 */
	virtual bool PostLoadJsonData(
		const FEngineVersion& EngineVersion,
		const FJsonDataCustomVersions& CustomVersions,
		TSharedRef<FJsonObject> JsonObject);

protected:
	// Check whether we need to handle the given PostRename event. This can return false e.g. if the object is only
	// being reinstanced.
	bool MustHandleRename(UObject* OldOuter, const FName OldName) const;

	// Get a list of custom version identifiers used by this object
	virtual TSet<FGuid> GetRelevantCustomVersions() const;

private:
	// The actual loading logic that takes care of creating UObjects. Call the json property load internally.
	static UJsonDataAsset* LoadJsonDataAsset_Internal(FJsonDataAssetPath Path, UJsonDataAsset* ExistingDataAsset);

public:
	// - UObject interface
	bool Rename(const TCHAR* NewName = nullptr, UObject* NewOuter = nullptr, ERenameFlags Flags = REN_None) override;
	void PostRename(UObject* OldOuter, const FName OldName) override;

	void PostSaveRoot(FObjectPostSaveRootContext ObjectSaveContext) override;
	// Not called for newly created objects
	void PostLoad() override;
	void PostDuplicate(bool bDuplicateForPIE) override;

	bool IsFullNameStableForNetworking() const override;
	bool IsSupportedForNetworking() const override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) override;
#endif

private:
	bool bIsInPostLoad = false;
};
