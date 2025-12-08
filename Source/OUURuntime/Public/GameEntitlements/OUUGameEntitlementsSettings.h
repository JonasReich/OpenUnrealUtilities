// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameEntitlements/OUUGameEntitlementsTags.h"

#include "OUUGameEntitlementsSettings.generated.h"

#if WITH_EDITOR
DECLARE_MULTICAST_DELEGATE_OneParam(FOnOUUGameEntitlementSettingsChanged, FPropertyChangedChainEvent&);
#endif

UCLASS(BlueprintType, Config = "Game", DefaultConfig)
class OUURUNTIME_API UOUUGameEntitlementSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static const UOUUGameEntitlementSettings& Get() { return *GetDefault<UOUUGameEntitlementSettings>(); }

// - UObject
#if WITH_EDITOR
	void PostEditChangeChainProperty(struct FPropertyChangedChainEvent& PropertyChangedEvent) override
	{
		Super::PostEditChangeChainProperty(PropertyChangedEvent);
		OnSettingsChanged.Broadcast(PropertyChangedEvent);
	}
#endif

	// Which version to apply if nothing is overridden from command line or console variables.
	UPROPERTY(Config, EditAnywhere)
	FOUUGameEntitlementVersion DefaultVersion;

	// Which version to apply in editor if nothing is overridden from command line or console variables.
	UPROPERTY(Config, EditAnywhere)
	FOUUGameEntitlementVersion DefaultEditorVersion;

	// Enable an extension for the PIE toolbar to show current OVERRIDE entitlement version.
	// Recommended to be used if your PIE testing needs frequent tests with different entitlement versions.
	UPROPERTY(Config, EditAnywhere, meta = (ConfigRestartRequired = true))
	bool EnablePIEToolbarExtension = false;

	UPROPERTY(Config, EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementModuleAndCollection}"))
	TMap<FOUUGameEntitlementVersion, FGameplayTagContainer> EntitlementsPerVersion;

	UPROPERTY(Config, EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementModule}"))
	TMap<FOUUGameEntitlementCollection, FGameplayTagContainer> ModuleCollections;

#if WITH_EDITOR
	FOnOUUGameEntitlementSettingsChanged OnSettingsChanged;
#endif
};
