// Copyright (c) 2026 Jonas Reich & Contributors

#include "AbilitySystemComponent.h"
#include "ActorMapWindow/OUUActorMapWindow_TabSpawner.h"
#include "Misc/RegexUtils.h"
#include "WorldPartition/WorldPartitionActorDescInstance.h"

namespace OUU::Developer::ActorMapWindow
{
	template <typename ExitClass>
	bool ClassMatchesSearchString(const UClass* Class, const FString& SearchName)
	{
		if (!IsValid(Class))
			return false;

		// Iterate through all parent classes to find a match
		while (Class != UStruct::StaticClass() && Class != UClass::StaticClass() && Class != ExitClass::StaticClass())
		{
			if (Class->GetName().Equals(SearchName, ESearchCase::IgnoreCase))
				return true;
			Class = Class->GetSuperClass();
		}

		return false;
	}
} // namespace OUU::Developer::ActorMapWindow

bool FOUUActorMapQuery::MatchesActor(const AActor* Actor) const
{
	if (!IsValid(Actor))
		return false;

	bool bAtLeastOneFilterActive = false;

	const FString ActorName = Actor->GetActorNameOrLabel();
	if (!NameFilter.IsEmpty())
	{
		bAtLeastOneFilterActive = true;
		if (!ActorName.Contains(NameFilter))
			return false;
	}

	if (!NameRegexPattern.IsEmpty())
	{
		bAtLeastOneFilterActive = true;
		if (!OUU::Runtime::RegexUtils::MatchesRegex(NameRegexPattern, ActorName))
			return false;
	}

	if (ActorClassName.IsEmpty() == false)
	{
		bAtLeastOneFilterActive = true;

		if (ResolvedActorClass == nullptr && bClassNotResolved)
		{
			ResolvedActorClass = UClass::TryFindTypeSlow<UClass>(*ActorClassName);
			bClassNotResolved = false;
		}
		if (Actor->GetClass()->IsChildOf(ResolvedActorClass) == false)
		{
			return false;
		}
	}

	if (ComponentClassName.IsEmpty() == false)
	{
		bAtLeastOneFilterActive = true;
		bool bAtLeastOneComponentMatches = false;
		for (auto& Component : Actor->GetComponents())
		{
			if (OUU::Developer::ActorMapWindow::ClassMatchesSearchString<UActorComponent>(
					Component->GetClass(),
					ComponentClassName))
			{
				bAtLeastOneComponentMatches = true;
				break;
			}
		}
		if (!bAtLeastOneComponentMatches)
			return false;
	}

	if (!ActorTagQuery.IsEmpty())
	{
		bAtLeastOneFilterActive = true;
		if (const UAbilitySystemComponent* AbilitySystemComponent =
				Actor->FindComponentByClass<UAbilitySystemComponent>())
		{
			FGameplayTagContainer OwnedTags;
			AbilitySystemComponent->GetOwnedGameplayTags(OUT OwnedTags);
			if (!ActorTagQuery.Matches(OwnedTags))
				return false;
		}
		else
		{
			return false;
		}
	}

	return bAtLeastOneFilterActive;
}

#if WITH_EDITOR
bool FOUUActorMapQuery::MatchesActorDescription(const FWorldPartitionActorDescInstance& _ActorDesc) const
{
	bool bAtLeastOneFilterActive = false;

	const FString& ActorName = _ActorDesc.GetActorLabelString();
	if (!NameFilter.IsEmpty())
	{
		bAtLeastOneFilterActive = true;
		if (!ActorName.Contains(NameFilter))
			return false;
	}

	if (!NameRegexPattern.IsEmpty())
	{
		bAtLeastOneFilterActive = true;
		if (!OUU::Runtime::RegexUtils::MatchesRegex(NameRegexPattern, ActorName))
			return false;
	}

	if (ActorClassName.IsEmpty() == false)
	{
		bAtLeastOneFilterActive = true;

		if (ResolvedActorClass == nullptr && bClassNotResolved)
		{
			ResolvedActorClass = UClass::TryFindTypeSlow<UClass>(*ActorClassName);
			bClassNotResolved = false;
		}
		if (_ActorDesc.GetActorNativeClass()->IsChildOf(ResolvedActorClass) == false)
		{
			return false;
		}
	}

	return bAtLeastOneFilterActive;
}
#endif
