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
	static UOUUGameEntitlementsSubsystem& Get();

	UFUNCTION(BlueprintPure)
	bool IsEntitled(const FOUUGameEntitlementModule& Module) const;
	bool IsEntitled(const FOUUGameEntitlementModules_Ref& Modules) const;

	FOUUGameEntitlementModules_Ref GetActiveEntitlements() const;

	UFUNCTION(BlueprintPure, DisplayName = "GetActiveEntitlements")
	FGameplayTagContainer K2_GetActiveEntitlements() const;

	UFUNCTION(BlueprintPure)
	FOUUGameEntitlementVersion GetActiveVersion() const;

	// Restrict Blueprint access for now.
	void SetOverrideVersion(const FOUUGameEntitlementVersion& Version);

	// - USubsystem
	void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	void RefreshActiveVersionAndEntitlements();

	FOUUGameEntitlementVersion OverrideVersion;
	FOUUGameEntitlementVersion ActiveVersion;
	FOUUGameEntitlementModules_Value ActiveEntitlements;
};
