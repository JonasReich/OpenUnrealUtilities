// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "Engine/DeveloperSettings.h"
#include "GameplayTags/TypedGameplayTag.h"
#include "Subsystems/EngineSubsystem.h"

#include "OUUGameEntitlements.generated.h"

OUU_DECLARE_GAMEPLAY_TAGS_START(OUURUNTIME_API, FOUUGameEntitlementTags, "GameEntitlements", "")
	OUU_GTAG(
		Module,
		"Inidividual modules that can be locked/unlocked",
		ParentTagType::Flags | EFlags::AllowContentChildTags)
	OUU_GTAG(
		Collection,
		"Meta combination of modules that can be controlled at once",
		ParentTagType::Flags | EFlags::AllowContentChildTags)
	OUU_GTAG(
		Version,
		"A 'version' of the game that has a predefined selection of modules that are available",
		ParentTagType::Flags | EFlags::AllowContentChildTags)
OUU_DECLARE_GAMEPLAY_TAGS_END(FOUUGameEntitlementTags)

USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUGameEntitlementModule : public FTypedGameplayTag_Base
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_GAMEPLAY_TAG(
		FOUUGameEntitlementModule,
		FOUUGameEntitlementTags::Module,
		FOUUGameEntitlementTags::Collection)
};

USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUGameEntitlementCollection : public FTypedGameplayTag_Base
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_GAMEPLAY_TAG(FOUUGameEntitlementCollection, FOUUGameEntitlementTags::Collection)
};

USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUGameEntitlementVersion : public FTypedGameplayTag_Base
{
	GENERATED_BODY()
	IMPLEMENT_TYPED_GAMEPLAY_TAG(FOUUGameEntitlementVersion, FOUUGameEntitlementTags::Version)
};

UCLASS(BlueprintType, Config = "Game", DefaultConfig)
class OUURUNTIME_API UOUUGameEntitlementSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static const UOUUGameEntitlementSettings& Get();

	// Which version to apply if nothing is overridden from command line or console variables.
	UPROPERTY(Config, EditAnywhere)
	FOUUGameEntitlementVersion DefaultVersion;

	UPROPERTY(Config, EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementModule}"))
	TMap<FOUUGameEntitlementVersion, FGameplayTagContainer> EntitlementsPerVersion;

	UPROPERTY(Config, EditAnywhere, meta = (Categories = "TypedTag{OUUGameEntitlementModule}"))
	TMap<FOUUGameEntitlementCollection, FGameplayTagContainer> ModuleCollections;
};

// Central subsystem to track entitlements
UCLASS()
class OUURUNTIME_API UOUUGameEntitlementsSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
	static UOUUGameEntitlementsSubsystem& Get();

	bool IsEntitled(const FOUUGameEntitlementModule& Module) const;
	bool IsEntitled(const FOUUGameEntitlementModules_Ref& Modules) const;

	void SetOverrideVersion(const FOUUGameEntitlementVersion& Version);

	// - USubsystem
	void Initialize(FSubsystemCollectionBase& Collection) override;

private:
	void RefreshActiveVersionAndEntitlements();

	FOUUGameEntitlementVersion OverrideVersion;
	FOUUGameEntitlementVersion ActiveVersion;
	FOUUGameEntitlementModules_Value ActiveEntitlements;
};
