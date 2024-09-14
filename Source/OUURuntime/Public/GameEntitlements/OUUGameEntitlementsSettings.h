// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameEntitlements/OUUGameEntitlementsTags.h"

#include "OUUGameEntitlementsSettings.generated.h"

UCLASS(BlueprintType, Config = "Game", DefaultConfig)
class OUURUNTIME_API UOUUGameEntitlementSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static const UOUUGameEntitlementSettings& Get() { return *GetDefault<UOUUGameEntitlementSettings>(); }

	// Which version to apply if nothing is overridden from command line or console variables.
	UPROPERTY(Config, EditAnywhere)
	FOUUGameEntitlementVersion DefaultVersion;

	UPROPERTY(Config, EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementModule}"))
	TMap<FOUUGameEntitlementVersion, FGameplayTagContainer> EntitlementsPerVersion;

	UPROPERTY(Config, EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementModule}"))
	TMap<FOUUGameEntitlementCollection, FGameplayTagContainer> ModuleCollections;
};
