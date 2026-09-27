// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameEntitlements/OUUGameEntitlementsTags.h"

#include "OUUGameEntitlementsSettings.generated.h"

#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOUUGameEntitlementSettingsChanged, FPropertyChangedChainEvent&);
#endif

/** A single entitlement (module or collection) and every version, collection and DLC it is explicitly assigned to. */
USTRUCT()
struct OUURUNTIME_API FOUUGameEntitlementAssignment
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementModuleAndCollection}"))
	FGameplayTag Entitlement;

	UPROPERTY(EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementVersion}"))
	FGameplayTagContainer Versions;

	UPROPERTY(EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementCollection}"))
	FGameplayTagContainer Collections;

	// Steam DLC AppIDs that grant this entitlement while owned/installed.
	UPROPERTY(EditAnywhere)
	TArray<int32> SteamDlcAppIds;

	bool HasAnyAssignment() const { return Versions.Num() > 0 || Collections.Num() > 0 || SteamDlcAppIds.Num() > 0; }
};

USTRUCT()
struct OUURUNTIME_API FOUUSteamDlcInfo
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	int32 AppId = 0;

	// Only used for display in the editor.
	UPROPERTY(EditAnywhere)
	FString DisplayName;
};

UCLASS(BlueprintType, Config = "Game", DefaultConfig)
class OUURUNTIME_API UOUUGameEntitlementSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static const UOUUGameEntitlementSettings& Get() { return *GetDefault<UOUUGameEntitlementSettings>(); }

	// Entitlements explicitly assigned per version. Built from Entitlements.
	const TMap<FGameplayTag, FGameplayTagContainer>& GetEntitlementsPerVersion() const { return VersionLookup; }

	// Entitlements explicitly contained per collection. Built from Entitlements.
	const TMap<FGameplayTag, FGameplayTagContainer>& GetModuleCollections() const { return CollectionLookup; }

	// Entitlements granted per Steam DLC AppID. Built from Entitlements.
	const TMap<int32, FGameplayTagContainer>& GetSteamDlcEntitlements() const { return SteamDlcLookup; }

	// - UObject
	void PostInitProperties() override;
	void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
#if WITH_EDITOR
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override;
	void PostEditUndo() override;
#endif

public:
	// Which version to apply if nothing is overridden from command line or console variables.
	UPROPERTY(Config, EditAnywhere)
	FOUUGameEntitlementVersion DefaultVersion;

	// Which version to apply in editor if nothing is overridden from command line or console variables.
	UPROPERTY(Config, EditAnywhere)
	FOUUGameEntitlementVersion DefaultEditorVersion;

	// Enable an extension for the PIE toolbar to show current OVERRIDE entitlement version.
	// Recommended to be used if your PIE testing needs frequent tests with different entitlement versions.
	// #TODO jreich: This only affects editor UI, but it lives in shared project config, so each developer cannot pick
	// their own value. Remove it once the toolbar entry is registered by default.
	UPROPERTY(Config, EditAnywhere, meta = (ConfigRestartRequired = true))
	bool EnablePIEToolbarExtension = false;

	// Steam DLCs that can grant entitlements. Each entry is a column in the entitlements grid.
	UPROPERTY(Config, EditAnywhere)
	TArray<FOUUSteamDlcInfo> SteamDlcs;

	// One entry per entitlement with at least one assignment. Edited through the entitlements grid.
	UPROPERTY(Config, EditAnywhere)
	TArray<FOUUGameEntitlementAssignment> Entitlements;

#if WITH_EDITOR
	FOnOUUGameEntitlementSettingsChanged OnSettingsChanged;
#endif

private:
	void RebuildLookups();

	// Imports the map based config layout used before Entitlements existed. Only runs while Entitlements is empty.
	void MigrateLegacyConfig();

#if WITH_EDITOR
	// Sorts rows and their contents and drops rows without assignments, so the saved ini stays stable.
	void CanonicalizeEntitlements();
#endif

	// The lookups are UPROPERTYs so the legacy config keys can be imported into them via ImportText.
	UPROPERTY(Transient)
	TMap<FGameplayTag, FGameplayTagContainer> VersionLookup;

	UPROPERTY(Transient)
	TMap<FGameplayTag, FGameplayTagContainer> CollectionLookup;

	UPROPERTY(Transient)
	TMap<int32, FGameplayTagContainer> SteamDlcLookup;
};
