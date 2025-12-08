// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameEntitlements/OUUGameEntitlementsTags.h"
#include "Subsystems/EngineSubsystem.h"

#include "OUUGameEntitlements.generated.h"

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

	// - USubsystem
	void Initialize(FSubsystemCollectionBase& Collection) override;

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

	bool bHasInitializedActiveEntitlements = false;

	UPROPERTY(EditAnywhere)
	FOUUGameEntitlementVersion OverrideVersion;

	FOUUGameEntitlementVersion ActiveVersion;
	FOUUGameEntitlementModuleAndCollections_Value ActiveEntitlements;
};
