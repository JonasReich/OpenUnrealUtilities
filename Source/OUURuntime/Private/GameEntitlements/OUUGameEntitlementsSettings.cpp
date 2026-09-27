// Copyright (c) 2024 Jonas Reich & Contributors

#include "GameEntitlements/OUUGameEntitlementsSettings.h"

#include "LogOpenUnrealUtilities.h"
#include "Misc/ConfigCacheIni.h"
#include "UObject/UnrealType.h"

#if WITH_EDITOR
namespace OUU::Runtime::GameEntitlements::Private
{
	FGameplayTagContainer MakeSortedContainer(const FGameplayTagContainer& Tags)
	{
		TArray<FGameplayTag> SortedTags = Tags.GetGameplayTagArray();
		SortedTags.Sort(
			[](const FGameplayTag& A, const FGameplayTag& B) { return A.GetTagName().LexicalLess(B.GetTagName()); });
		return FGameplayTagContainer::CreateFromArray(SortedTags);
	}
} // namespace OUU::Runtime::GameEntitlements::Private
#endif

void UOUUGameEntitlementSettings::PostInitProperties()
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		MigrateLegacyConfig();
	}
	RebuildLookups();
}

void UOUUGameEntitlementSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);
	RebuildLookups();
}

#if WITH_EDITOR
void UOUUGameEntitlementSettings::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	CanonicalizeEntitlements();
	RebuildLookups();

	Super::PostEditChangeChainProperty(PropertyChangedEvent);
	OnSettingsChanged.Broadcast(PropertyChangedEvent);
}

void UOUUGameEntitlementSettings::PostEditUndo()
{
	Super::PostEditUndo();
	RebuildLookups();

	// UObject::PostEditUndo only routes to PostEditChangeProperty, so listeners are notified here.
	FEditPropertyChain PropertyChain;
	FPropertyChangedEvent PropertyEvent(nullptr);
	FPropertyChangedChainEvent ChainEvent(PropertyChain, PropertyEvent);
	OnSettingsChanged.Broadcast(ChainEvent);
}
#endif

void UOUUGameEntitlementSettings::RebuildLookups()
{
	VersionLookup.Reset();
	CollectionLookup.Reset();
	SteamDlcLookup.Reset();

	for (const auto& Row : Entitlements)
	{
		if (Row.Entitlement.IsValid() == false)
		{
			continue;
		}

		for (const auto& Version : Row.Versions)
		{
			VersionLookup.FindOrAdd(Version).AddTag(Row.Entitlement);
		}
		for (const auto& Collection : Row.Collections)
		{
			CollectionLookup.FindOrAdd(Collection).AddTag(Row.Entitlement);
		}
		for (const int32 AppId : Row.SteamDlcAppIds)
		{
			SteamDlcLookup.FindOrAdd(AppId).AddTag(Row.Entitlement);
		}
	}
}

void UOUUGameEntitlementSettings::MigrateLegacyConfig()
{
	if (Entitlements.Num() > 0)
	{
		return;
	}

	const FString IniSectionName = GetClass()->GetPathName();
	const FString ConfigName = GetClass()->GetConfigName();

	bool bFoundLegacyConfig = false;
	const auto ImportLegacyKey = [&](const TCHAR* LegacyKey, FName LookupPropertyName) {
		FString LegacyText;
		if (GConfig->GetString(*IniSectionName, LegacyKey, LegacyText, ConfigName) == false)
		{
			return;
		}

		const FProperty* LookupProperty = FindFProperty<FProperty>(GetClass(), LookupPropertyName);
		if (ensure(LookupProperty))
		{
			LookupProperty->ImportText_InContainer(*LegacyText, this, this, PPF_None);
			bFoundLegacyConfig = true;
		}
	};

	ImportLegacyKey(
		TEXT("EntitlementsPerVersion"),
		GET_MEMBER_NAME_CHECKED(UOUUGameEntitlementSettings, VersionLookup));
	ImportLegacyKey(TEXT("ModuleCollections"), GET_MEMBER_NAME_CHECKED(UOUUGameEntitlementSettings, CollectionLookup));
	ImportLegacyKey(TEXT("SteamDlcEntitlements"), GET_MEMBER_NAME_CHECKED(UOUUGameEntitlementSettings, SteamDlcLookup));

	if (bFoundLegacyConfig == false)
	{
		return;
	}

	TMap<FGameplayTag, FOUUGameEntitlementAssignment> RowsByTag;
	const auto FindOrAddRow = [&RowsByTag](const FGameplayTag& Tag) -> FOUUGameEntitlementAssignment& {
		auto& Row = RowsByTag.FindOrAdd(Tag);
		Row.Entitlement = Tag;
		return Row;
	};

	for (const auto& Entry : VersionLookup)
	{
		for (const auto& Tag : Entry.Value)
		{
			FindOrAddRow(Tag).Versions.AddTag(Entry.Key);
		}
	}
	for (const auto& Entry : CollectionLookup)
	{
		for (const auto& Tag : Entry.Value)
		{
			FindOrAddRow(Tag).Collections.AddTag(Entry.Key);
		}
	}
	for (const auto& Entry : SteamDlcLookup)
	{
		if (SteamDlcs.ContainsByPredicate([&Entry](const FOUUSteamDlcInfo& Dlc) { return Dlc.AppId == Entry.Key; })
			== false)
		{
			FOUUSteamDlcInfo& Dlc = SteamDlcs.AddDefaulted_GetRef();
			Dlc.AppId = Entry.Key;
		}

		for (const auto& Tag : Entry.Value)
		{
			FindOrAddRow(Tag).SteamDlcAppIds.AddUnique(Entry.Key);
		}
	}

	RowsByTag.GenerateValueArray(Entitlements);
#if WITH_EDITOR
	CanonicalizeEntitlements();
#endif

	UE_LOG(
		LogOpenUnrealUtilities,
		Warning,
		TEXT("Migrated legacy game entitlement config keys (EntitlementsPerVersion, ModuleCollections, "
			 "SteamDlcEntitlements) in [%s] to %d Entitlements rows. Save the entitlement settings and remove the "
			 "legacy keys from the ini."),
		*IniSectionName,
		Entitlements.Num());
}

#if WITH_EDITOR
void UOUUGameEntitlementSettings::CanonicalizeEntitlements()
{
	using namespace OUU::Runtime::GameEntitlements::Private;

	Entitlements.RemoveAll([](const FOUUGameEntitlementAssignment& Row) {
		return Row.Entitlement.IsValid() == false || Row.HasAnyAssignment() == false;
	});

	for (auto& Row : Entitlements)
	{
		Row.Versions = MakeSortedContainer(Row.Versions);
		Row.Collections = MakeSortedContainer(Row.Collections);

		Row.SteamDlcAppIds.Sort();
		for (int32 Idx = Row.SteamDlcAppIds.Num() - 1; Idx > 0; --Idx)
		{
			if (Row.SteamDlcAppIds[Idx] == Row.SteamDlcAppIds[Idx - 1])
			{
				Row.SteamDlcAppIds.RemoveAt(Idx);
			}
		}
	}

	Entitlements.Sort([](const FOUUGameEntitlementAssignment& A, const FOUUGameEntitlementAssignment& B) {
		return A.Entitlement.GetTagName().LexicalLess(B.Entitlement.GetTagName());
	});
}
#endif
