// Copyright (c) 2024 Jonas Reich & Contributors

#include "GameEntitlements/OUUGameEntitlementsResolver.h"

#include "GameEntitlements/OUUGameEntitlementsSettings.h"
#include "GameEntitlements/OUUGameEntitlementsTags.h"

namespace OUU::Runtime::GameEntitlements
{
	FGameplayTagContainer ResolveEntitlements(
		const UOUUGameEntitlementSettings& Settings,
		const FGameplayTagContainer& SeedTags,
		bool bAllowUnlockAllDlc,
		FEntitlementSourceMap* OutSources)
	{
		FGameplayTagContainer Result;
		TArray<FGameplayTag> PendingTags;

		const auto AddTag = [&](const FGameplayTag& Tag, const FEntitlementSource& Source) {
			if (OutSources)
			{
				OutSources->FindOrAdd(Tag).AddUnique(Source);
			}
			if (Result.HasTagExact(Tag) == false)
			{
				Result.AddTag(Tag);
				PendingTags.Add(Tag);
			}
		};

		for (const auto& Tag : SeedTags)
		{
			AddTag(Tag, FEntitlementSource{EEntitlementSourceType::Explicit, FGameplayTag()});
		}

		const FGameplayTag UnlockAllDlcTag = FOUUGameEntitlementTags::Module::UnlockAllDLC::GetTag();
		for (int32 Idx = 0; Idx < PendingTags.Num(); ++Idx)
		{
			const FGameplayTag Tag = PendingTags[Idx];

			if (const auto* CollectionContents = Settings.GetModuleCollections().Find(Tag))
			{
				for (const auto& ContainedTag : *CollectionContents)
				{
					AddTag(ContainedTag, FEntitlementSource{EEntitlementSourceType::Collection, Tag});
				}
			}

			if (bAllowUnlockAllDlc && Tag == UnlockAllDlcTag)
			{
				for (const auto& DlcEntry : Settings.GetSteamDlcEntitlements())
				{
					for (const auto& DlcTag : DlcEntry.Value)
					{
						AddTag(DlcTag, FEntitlementSource{EEntitlementSourceType::UnlockAllDlc, Tag});
					}
				}
			}
		}

		return Result;
	}
} // namespace OUU::Runtime::GameEntitlements
