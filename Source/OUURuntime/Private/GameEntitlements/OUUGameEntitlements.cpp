// Copyright (c) 2024 Jonas Reich & Contributors

#include "GameEntitlements/OUUGameEntitlements.h"

#include "Engine/World.h"
#include "GameEntitlements/OUUGameEntitlementsSettings.h"
#include "HAL/IConsoleManager.h"
#include "LogOpenUnrealUtilities.h"
#include "Online/OUUSteamUtils.h"

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

#if !UE_BUILD_SHIPPING
TAutoConsoleVariable<bool> CVar_UnlockAllDlcEntitlements{
	TEXT("ouu.Entitlements.UnlockAllDlc"),
	false,
	TEXT("Editor/testing only: treat all configured Steam DLC as owned so their entitlement modules are granted.")};

namespace OUU::Runtime::GameEntitlements::Private
{
	void SetDlcOverrideFromConsole(const TArray<FString>& Args, bool bForceUnlocked)
	{
		const TCHAR* CommandName =
			bForceUnlocked ? TEXT("ouu.Entitlements.AddDlcOverride") : TEXT("ouu.Entitlements.RemoveDlcOverride");
		if (Args.Num() < 1 || Args[0].IsNumeric() == false)
		{
			UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("Usage: %s <SteamDlcAppId>"), CommandName);
			return;
		}

		if (GEngine == nullptr)
		{
			return;
		}

		auto* Subsystem = GEngine->GetEngineSubsystem<UOUUGameEntitlementsSubsystem>();
		if (Subsystem == nullptr)
		{
			return;
		}

		Subsystem->SetDlcForcedUnlocked(FCString::Atoi(*Args[0]), bForceUnlocked);
	}
} // namespace OUU::Runtime::GameEntitlements::Private

FAutoConsoleCommand GConsoleCommand_AddDlcOverride{
	TEXT("ouu.Entitlements.AddDlcOverride"),
	TEXT("Editor/testing only: add a single Steam DLC (by AppID) to the force-unlock override set so it is treated as "
		 "owned. Ignored while ouu.Entitlements.UnlockAllDlc is set."),
	FConsoleCommandWithArgsDelegate::CreateStatic(
		&OUU::Runtime::GameEntitlements::Private::SetDlcOverrideFromConsole,
		/*bForceUnlocked =*/true)};

FAutoConsoleCommand GConsoleCommand_RemoveDlcOverride{
	TEXT("ouu.Entitlements.RemoveDlcOverride"),
	TEXT(
		"Editor/testing only: remove a single Steam DLC (by AppID) from the force-unlock override set, reverting it to "
		"its real ownership state."),
	FConsoleCommandWithArgsDelegate::CreateStatic(
		&OUU::Runtime::GameEntitlements::Private::SetDlcOverrideFromConsole,
		/*bForceUnlocked =*/false)};
#endif

UOUUGameEntitlementsSubsystem& UOUUGameEntitlementsSubsystem::Get()
{
	return *GEngine->GetEngineSubsystem<UOUUGameEntitlementsSubsystem>();
}

bool UOUUGameEntitlementsSubsystem::IsEntitled(
	const FOUUGameEntitlementModuleAndCollections_Value& ActiveEntitlements,
	const FOUUGameEntitlementModule& Module)
{
	// Invalid = empty tag should be treated as asking for "no requirements"
	return Module.IsValid() == false
		|| ActiveEntitlements.HasTag(FOUUGameEntitlementModuleAndCollection::ConvertChecked(Module));
}

bool UOUUGameEntitlementsSubsystem::IsEntitled(const FOUUGameEntitlementModule& Module) const
{
	return IsEntitled(ActiveEntitlements, Module);
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

#if !UE_BUILD_SHIPPING
void UOUUGameEntitlementsSubsystem::SetDlcForcedUnlocked(int32 SteamDlcAppId, bool bForceUnlocked)
{
	bool bChanged;
	if (bForceUnlocked)
	{
		bool bWasAlreadyForced = false;
		ForcedUnlockedDlcAppIds.Add(SteamDlcAppId, &bWasAlreadyForced);
		bChanged = bWasAlreadyForced == false;

		if (UOUUGameEntitlementSettings::Get().SteamDlcEntitlements.Contains(SteamDlcAppId) == false)
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Warning,
				TEXT("AddDlcOverride: %d is not a configured Steam DLC AppID; the override will have no effect."),
				SteamDlcAppId);
		}
	}
	else
	{
		bChanged = ForcedUnlockedDlcAppIds.Remove(SteamDlcAppId) > 0;
	}

	if (bChanged)
	{
		RefreshActiveVersionAndEntitlements();
	}
}
#endif

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

	// Re-evaluate entitlements when a game instance starts, once Steam DLC ownership is available.
	StartGameInstanceHandle =
		FWorldDelegates::OnStartGameInstance.AddUObject(this, &UOUUGameEntitlementsSubsystem::HandleStartGameInstance);

#if !UE_BUILD_SHIPPING
	CVar_UnlockAllDlcEntitlements.AsVariable()->SetOnChangedCallback(
		FConsoleVariableDelegate::CreateUObject(this, &UOUUGameEntitlementsSubsystem::HandleUnlockAllDlcCVarChanged));
#endif
}

void UOUUGameEntitlementsSubsystem::Deinitialize()
{
	FWorldDelegates::OnStartGameInstance.Remove(StartGameInstanceHandle);
	StartGameInstanceHandle.Reset();

	Super::Deinitialize();
}

void UOUUGameEntitlementsSubsystem::HandleStartGameInstance(UGameInstance* /*GameInstance*/)
{
	RefreshActiveVersionAndEntitlements();
}

#if !UE_BUILD_SHIPPING
void UOUUGameEntitlementsSubsystem::HandleUnlockAllDlcCVarChanged(IConsoleVariable* /*Variable*/)
{
	RefreshActiveVersionAndEntitlements();
}
#endif

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

	// Grant entitlements for owned Steam DLC (the ownership query is wrapped in UOUUSteamUtils).
#if !UE_BUILD_SHIPPING
	const bool bUnlockAllDlc = CVar_UnlockAllDlcEntitlements.GetValueOnGameThread();
#else
	const bool bUnlockAllDlc = false;
#endif
	for (const auto& DlcEntry : Settings.SteamDlcEntitlements)
	{
#if !UE_BUILD_SHIPPING
		const bool bForceUnlocked = ForcedUnlockedDlcAppIds.Contains(DlcEntry.Key);
#else
		const bool bForceUnlocked = false;
#endif
		if (bUnlockAllDlc || bForceUnlocked || UOUUSteamUtils::IsDlcInstalled(DlcEntry.Key))
		{
			ActiveEntitlements.AppendTags(FOUUGameEntitlementModuleAndCollections_Value::CreateChecked(DlcEntry.Value));
		}
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
