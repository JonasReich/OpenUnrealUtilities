// Copyright (c) 2024 Jonas Reich & Contributors

#include "Gameentitlements/OUUGameEntitlements.h"

OUU_DEFINE_GAMEPLAY_TAGS(FOUUGameEntitlementTags)
DEFINE_TYPED_GAMEPLAY_TAG(FOUUGameEntitlementModule)
DEFINE_TYPED_GAMEPLAY_TAG(FOUUGameEntitlementCollection)
DEFINE_TYPED_GAMEPLAY_TAG(FOUUGameEntitlementVersion)

extern TAutoConsoleVariable<FString> CVar_OverrideEntitlementVersion;

namespace OUU::Runtime::GameEntitlements
{
	void UpdateOverrideEntitlementFromCVar()
	{
		auto TagName = FOUUGameEntitlementTags::Version::Get().GetName() + TEXT(".")
			+ CVar_OverrideEntitlementVersion.GetValueOnGameThread();

		const FGameplayTag RawTag = FGameplayTag::RequestGameplayTag(*TagName, false);
		const auto VersionTag = FOUUGameEntitlementVersion::TryConvert(RawTag);

		// It's okay or even expected to pass an invalid tag here, because empty/invalid tags will reset the override.
		UOUUGameEntitlementsSubsystem::Get().SetOverrideVersion(VersionTag);
	}
} // namespace OUU::Runtime::GameEntitlements

TAutoConsoleVariable<FString> CVar_OverrideEntitlementVersion{
	TEXT("ouu.Entitlements.OverrideVersion"),
	TEXT(""),
	TEXT("Set an override 'game version' for the entitlements. Format: string for version tag excluding the prefix, "
		 "e.g. 'Foo' for the 'GameEntitlements.Version.Foo' tag."),
	FConsoleVariableDelegate::CreateLambda(
		[](IConsoleVariable*) { OUU::Runtime::GameEntitlements::UpdateOverrideEntitlementFromCVar(); })};

const UOUUGameEntitlementSettings& UOUUGameEntitlementSettings::Get()
{
	return *GetDefault<UOUUGameEntitlementSettings>();
}

UOUUGameEntitlementsSubsystem& UOUUGameEntitlementsSubsystem::Get()
{
	return *GEngine->GetEngineSubsystem<UOUUGameEntitlementsSubsystem>();
}

bool UOUUGameEntitlementsSubsystem::IsEntitled(const FOUUGameEntitlementModule& Module) const
{
	// Invalid = empty tag should be treated as asking for "no requirements"
	return Module.IsValid() == false || ActiveEntitlements.HasTag(Module);
}

bool UOUUGameEntitlementsSubsystem::IsEntitled(const FOUUGameEntitlementModules_Ref& Modules) const
{
	// Expected to return true if Modules is empty
	return ActiveEntitlements.HasAll(Modules);
}

void UOUUGameEntitlementsSubsystem::SetOverrideVersion(const FOUUGameEntitlementVersion& Version)
{
	OverrideVersion = Version;
	RefreshActiveVersionAndEntitlements();
}

void UOUUGameEntitlementsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	OUU::Runtime::GameEntitlements::UpdateOverrideEntitlementFromCVar();
	RefreshActiveVersionAndEntitlements();
}

void UOUUGameEntitlementsSubsystem::RefreshActiveVersionAndEntitlements()
{
	auto& Settings = UOUUGameEntitlementSettings::Get();
	ActiveVersion = OverrideVersion.IsValid() ? OverrideVersion : Settings.DefaultVersion;
	ActiveEntitlements.Reset();
	if (auto* EntitlementsPtr = Settings.EntitlementsPerVersion.Find(ActiveVersion))
	{
		ActiveEntitlements = FOUUGameEntitlementModules_Value::CreateChecked(*EntitlementsPtr);
	}

	// recursively add entitlements from module collections
	int32 LastEntitlementCount = -1;
	while (ActiveEntitlements.Num() != LastEntitlementCount)
	{
		for (auto Entitlement : ActiveEntitlements)
		{
			auto EntitlementAsCollection = FOUUGameEntitlementCollection::TryConvert(Entitlement);
			if (EntitlementAsCollection.IsValid())
			{
				if (auto* EntitlementsPtr = Settings.ModuleCollections.Find(EntitlementAsCollection))
				{
					ActiveEntitlements.AppendTags(FOUUGameEntitlementModules_Value::CreateChecked(*EntitlementsPtr));
				}
			}
		}
		LastEntitlementCount = ActiveEntitlements.Num();
	}
}
