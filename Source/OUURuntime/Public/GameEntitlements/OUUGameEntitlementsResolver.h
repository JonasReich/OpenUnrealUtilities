// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayTagContainer.h"

class UOUUGameEntitlementSettings;

namespace OUU::Runtime::GameEntitlements
{
	enum class EEntitlementSourceType : uint8
	{
		// Part of the seed tags.
		Explicit,
		// Contained in the collection stored in FEntitlementSource::Via.
		Collection,
		// Granted by a Steam DLC because the UnlockAllDLC module is entitled.
		UnlockAllDlc,
	};

	struct FEntitlementSource
	{
		EEntitlementSourceType Type = EEntitlementSourceType::Explicit;
		FGameplayTag Via;

		friend bool operator==(const FEntitlementSource& A, const FEntitlementSource& B)
		{
			return A.Type == B.Type && A.Via == B.Via;
		}
	};

	using FEntitlementSourceMap = TMap<FGameplayTag, TArray<FEntitlementSource>>;

	// Expands the seed tags by the contents of all included collections (recursively) and, if bAllowUnlockAllDlc is
	// set, by the modules of every Steam DLC once the UnlockAllDLC module is included.
	// Optionally collects every way each resulting tag was reached.
	OUURUNTIME_API FGameplayTagContainer ResolveEntitlements(
		const UOUUGameEntitlementSettings& Settings,
		const FGameplayTagContainer& SeedTags,
		bool bAllowUnlockAllDlc,
		FEntitlementSourceMap* OutSources = nullptr);
} // namespace OUU::Runtime::GameEntitlements
