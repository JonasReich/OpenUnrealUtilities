// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameEntitlements/OUUGameEntitlementsTags.h"
#include "Subsystems/EngineSubsystem.h"

#include "OUUGameEntitlements.generated.h"

class IConsoleVariable;
class UGameInstance;

/** Central subsystem to track entitlements */
UCLASS(BlueprintType)
class OUURUNTIME_API UOUUGameEntitlementsSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()

public:
	friend class FGameplayDebuggerCategory_GameEntitlements;

public:
	static UOUUGameEntitlementsSubsystem& Get();

	UFUNCTION(BlueprintPure)
	bool IsEntitled(const FOUUGameEntitlementModule& Module) const;
	bool IsEntitled(const FOUUGameEntitlementModules_Ref& Modules) const;

	bool HasInitializedActiveEntitlements() const;
	FOUUGameEntitlementModules_Value GetActiveEntitlements() const;

	UFUNCTION(BlueprintPure, DisplayName = "GetActiveEntitlements")
	FGameplayTagContainer K2_GetActiveEntitlements() const;

	UFUNCTION(BlueprintPure)
	FOUUGameEntitlementVersion GetActiveVersion() const;

	// Restrict Blueprint access for now.
	void SetOverrideVersion(const FOUUGameEntitlementVersion& Version);

#if !UE_BUILD_SHIPPING
	// Editor/testing only: force a single Steam DLC (by AppID) on or off in the entitlement rebuild, independent of
	// real Steam ownership. Ignored while ouu.Entitlements.UnlockAllDlc is set.
	void SetDlcForcedUnlocked(int32 SteamDlcAppId, bool bForceUnlocked);
#endif

	// - USubsystem
	void Initialize(FSubsystemCollectionBase& Collection) override;
	void Deinitialize() override;

public:
	// Called when entitlements are first initialized or changed by setting an override version.
	FSimpleMulticastDelegate OnActiveEntitlementsChanged;

private:
	bool IsEntitledToCollection(const FOUUGameEntitlementCollection& Collection) const;
	bool IsEntitledToCollection(const FOUUGameEntitlementCollections_Ref& Collections) const;

#if WITH_EDITOR
	void OnSettingsChanged(FPropertyChangedChainEvent& PropertyChangedEvent);
	void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif
	void RefreshActiveVersionAndEntitlements();

	// Re-evaluate entitlements once a game instance starts, when Steam DLC ownership is available.
	void HandleStartGameInstance(UGameInstance* GameInstance);

	FDelegateHandle StartGameInstanceHandle;

#if !UE_BUILD_SHIPPING
	void HandleUnlockAllDlcCVarChanged(IConsoleVariable* Variable);

	// Steam DLC AppIDs forced to be treated as owned, independent of real ownership. Editor/testing only.
	TSet<int32> ForcedUnlockedDlcAppIds;
#endif

	bool bHasInitializedActiveEntitlements = false;

	UPROPERTY(EditAnywhere)
	FOUUGameEntitlementVersion OverrideVersion;

	FOUUGameEntitlementVersion ActiveVersion;
	FOUUGameEntitlementModuleAndCollections_Value ActiveEntitlements;
};
