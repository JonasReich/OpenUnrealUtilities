// Copyright (c) 2024 Jonas Reich & Contributors

#include "GameEntitlements/OUUGameEntitlements.h"

#include "GameEntitlements/OUUGameEntitlementsSettings.h"

extern TAutoConsoleVariable<FString> CVar_OverrideEntitlementVersion;

namespace OUU::Runtime::GameEntitlements
{
	FOUUGameEntitlementVersion GetOverrideEntitlement()
	{
		auto TagName = FOUUGameEntitlementTags::Version::Get().GetName() + TEXT(".")
			+ CVar_OverrideEntitlementVersion.GetValueOnGameThread();

		const FGameplayTag RawTag = FGameplayTag::RequestGameplayTag(*TagName, false);
		return FOUUGameEntitlementVersion::TryConvert(RawTag);
	}

	void UpdateOverrideEntitlementFromCVar()
	{
		// When using command line to set the cvar via
		//     -ini:Engine:[ConsoleVariables]:ouu.Entitlements.OverrideVersion=...
		// this may be called earlier than the gameplay tags manager is initialized. Requesting a gameplay tag anyways
		// screws with the tag load order, so instead we rely on the subsystem initialization below to call this.
		if (UGameplayTagsManager::GetIfAllocated() && GEngine)
		{
			// It's okay or even expected to pass an invalid tag here, because empty/invalid tags will reset the
			// override.
			UOUUGameEntitlementsSubsystem::Get().SetOverrideVersion(
				OUU::Runtime::GameEntitlements::GetOverrideEntitlement());
		}
	}
} // namespace OUU::Runtime::GameEntitlements

TAutoConsoleVariable<FString> CVar_OverrideEntitlementVersion{
	TEXT("ouu.Entitlements.OverrideVersion"),
	TEXT(""),
	TEXT("Set an override 'game version' for the entitlements. Format: string for version tag excluding the prefix, "
		 "e.g. 'Foo' for the 'GameEntitlements.Version.Foo' tag."),
	FConsoleVariableDelegate::CreateLambda(
		[](IConsoleVariable*) { OUU::Runtime::GameEntitlements::UpdateOverrideEntitlementFromCVar(); })};

UOUUGameEntitlementsSubsystem& UOUUGameEntitlementsSubsystem::Get()
{
	return *GEngine->GetEngineSubsystem<UOUUGameEntitlementsSubsystem>();
}

bool UOUUGameEntitlementsSubsystem::IsEntitled(const FOUUGameEntitlementModule& Module) const
{
	// Invalid = empty tag should be treated as asking for "no requirements"
	return Module.IsValid() == false
		|| ActiveEntitlements.HasTag(FOUUGameEntitlementModuleAndCollection::ConvertChecked(Module));
}

bool UOUUGameEntitlementsSubsystem::IsEntitled(const FOUUGameEntitlementModules_Ref& Modules) const
{
	// Expected to return true if Modules is empty
	return ActiveEntitlements.HasAll(FOUUGameEntitlementModuleAndCollections_Value::CreateChecked(Modules.Get()));
}

bool UOUUGameEntitlementsSubsystem::HasInitializedActiveEntitlements() const
{
	return bHasInitializedActiveEntitlements;
}

FOUUGameEntitlementModules_Value UOUUGameEntitlementsSubsystem::GetActiveEntitlements() const
{
	return FOUUGameEntitlementModules_Value::CreateFiltered(ActiveEntitlements.Get());
}

FGameplayTagContainer UOUUGameEntitlementsSubsystem::K2_GetActiveEntitlements() const
{
	return ActiveEntitlements.Get();
}

FOUUGameEntitlementVersion UOUUGameEntitlementsSubsystem::GetActiveVersion() const
{
	return ActiveVersion;
}

void UOUUGameEntitlementsSubsystem::SetOverrideVersion(const FOUUGameEntitlementVersion& Version)
{
#if WITH_EDITOR
	FScopedTransaction Transaction(INVTEXT("Change Entitlement Override Version"));
	Modify();
#endif

	OverrideVersion = Version;
	if (bHasInitializedActiveEntitlements)
	{
		RefreshActiveVersionAndEntitlements();
	}
}

void UOUUGameEntitlementsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	checkf(UGameplayTagsManager::GetIfAllocated(), TEXT("Entitlements subsystem needs valid gameplay tags manager"));

	UGameplayTagsManager::Get().CallOrRegister_OnDoneAddingNativeTagsDelegate(
		FSimpleMulticastDelegate::FDelegate::CreateUObject(
			this,
			&UOUUGameEntitlementsSubsystem::RefreshActiveVersionAndEntitlements));

#if WITH_EDITOR
	GetMutableDefault<UOUUGameEntitlementSettings>()
		->OnSettingsChanged.AddUObject(this, &UOUUGameEntitlementsSubsystem::OnSettingsChanged);
#endif
}

bool UOUUGameEntitlementsSubsystem::IsEntitledToCollection(const FOUUGameEntitlementCollection& Collection) const
{
	// Invalid = empty tag should be treated as asking for "no requirements"
	return Collection.IsValid() == false
		|| ActiveEntitlements.HasTag(FOUUGameEntitlementModuleAndCollection::ConvertChecked(Collection));
}

bool UOUUGameEntitlementsSubsystem::IsEntitledToCollection(const FOUUGameEntitlementCollections_Ref& Collections) const
{
	// Expected to return true if Modules is empty
	return ActiveEntitlements.HasAll(FOUUGameEntitlementModuleAndCollections_Value::CreateChecked(Collections.Get()));
}

#if WITH_EDITOR
void UOUUGameEntitlementsSubsystem::OnSettingsChanged(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	RefreshActiveVersionAndEntitlements();
}

void UOUUGameEntitlementsSubsystem::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RefreshActiveVersionAndEntitlements();
}
#endif

void UOUUGameEntitlementsSubsystem::RefreshActiveVersionAndEntitlements()
{
	// Prevent recursing into this function
	bHasInitializedActiveEntitlements = false;

	static FOUUGameEntitlementVersion CachedOverrideVersion;
	if (OUU::Runtime::GameEntitlements::GetOverrideEntitlement() != CachedOverrideVersion)
	{
		// Only update the override version with the CVar's value if it has changed
		OUU::Runtime::GameEntitlements::UpdateOverrideEntitlementFromCVar();
		CachedOverrideVersion = OverrideVersion;
	}

	auto& Settings = UOUUGameEntitlementSettings::Get();
#if WITH_EDITOR
	auto& DefaultVersion =
		GIsEditor && IsRunningCookCommandlet() == false ? Settings.DefaultEditorVersion : Settings.DefaultVersion;
#else
	auto& DefaultVersion = Settings.DefaultVersion;
#endif
	ActiveVersion = OverrideVersion.IsValid() ? OverrideVersion : DefaultVersion;
	ActiveEntitlements.Reset();
	if (auto* EntitlementsPtr = Settings.EntitlementsPerVersion.Find(ActiveVersion))
	{
		ActiveEntitlements = FOUUGameEntitlementModuleAndCollections_Value::CreateChecked(*EntitlementsPtr);
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
					ActiveEntitlements.AppendTags(
						FOUUGameEntitlementModuleAndCollections_Value::CreateChecked(*EntitlementsPtr));
				}
			}
		}
		LastEntitlementCount = ActiveEntitlements.Num();
	}

	bHasInitializedActiveEntitlements = true;
	OnActiveEntitlementsChanged.Broadcast();
}
