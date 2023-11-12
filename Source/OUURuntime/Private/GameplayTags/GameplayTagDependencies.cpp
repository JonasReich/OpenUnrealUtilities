// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/GameplayTagDependencies.h"

#include "LogOpenUnrealUtilities.h"
#include "Templates/InterfaceUtils.h"

void IGameplayTagDependencyInterface::AppendTags(FGameplayTagContainer& OutTags) const
{
	// This tags split is just internal for faster propagation + debugging purposes.
	// From the outside we only ever care about all tags.
	AppendTags_Internal(OUT OutTags, EGameplayTagDependencyGetMode::AllTags, true, true);
}

void IGameplayTagDependencyInterface::BroadcastTagsChanged()
{
	FGameplayTagContainer AllTagsBefore = CachedTags_All;
	UpdateCachedTags(EGameplayTagDependencyGetMode::OwnTags);
	UpdateCachedTags(EGameplayTagDependencyGetMode::AllTags);

	BroadcastTagsChanged_Recursive(AllTagsBefore);
}

void IGameplayTagDependencyInterface::BindEventToOnTagsChanged(FGameplayTagDependencyEvent Event)
{
	OnTagsChanged.Add(Event);
}

void IGameplayTagDependencyInterface::UnbindEventFromOnTagsChanged(FGameplayTagDependencyEvent Event)
{
	OnTagsChanged.Remove(Event);
}

void IGameplayTagDependencyInterface::AddDependency(TScriptInterface<IGameplayTagDependencyInterface> Dependency)
{
	if (IsValidInterface<IGameplayTagDependencyInterface>(Dependency))
	{
		Dependencies.Add(TWeakInterfacePtr<IGameplayTagDependencyInterface>(Dependency.GetObject()));
		Dependency->ImmediateDependants.Add(GetInterfaceObject(this));
	}
}

void IGameplayTagDependencyInterface::RemoveDependency(TScriptInterface<IGameplayTagDependencyInterface> Dependency)
{
	if (IsValidInterface<IGameplayTagDependencyInterface>(Dependency))
	{
		Dependencies.Remove(TWeakInterfacePtr<IGameplayTagDependencyInterface>(Dependency.GetObject()));
		Dependency->ImmediateDependants.Remove(GetInterfaceObject(this));
	}
}

TMap<FGameplayTag, const UObject*> IGameplayTagDependencyInterface::GetImmediateTagSources() const
{
	TMap<FGameplayTag, const UObject*> Result;

	for (auto& Tag : CachedTags_All)
	{
		if (CachedTags_Own.HasTag(Tag))
		{
			Result.Add(Tag, GetInterfaceObject(this));
			continue;
		}

		bool bFoundTag = false;
		for (auto Dependency : Dependencies)
		{
			if (Dependency.IsValid() == false)
				continue;

			if (Dependency->CachedTags_All.HasTag(Tag))
			{
				Result.Add(Tag, GetInterfaceObject(Dependency.Get()));
				bFoundTag = true;
				break;
			}
		}

		UE_CLOG(
			bFoundTag == false,
			LogOpenUnrealUtilities,
			Warning,
			TEXT("%s - Tag %s from all tags list was found neither in own tags nor in dependency tags"),
			*GetNameSafe(GetInterfaceObject(this)),
			*Tag.ToString());
	}

	return Result;
}

void IGameplayTagDependencyInterface::GetOriginalTagSources(TMap<FGameplayTag, TSet<const UObject*>>& InOutResult) const
{
	for (auto Tag : CachedTags_Own)
	{
		InOutResult.FindOrAdd(Tag).Add(GetInterfaceObject(this));
	}

	for (auto Dependency : Dependencies)
	{
		if (Dependency.IsValid() == false)
			continue;
		Dependency->GetOriginalTagSources(IN OUT InOutResult);
	}
}

void IGameplayTagDependencyInterface::UpdateCachedTags(EGameplayTagDependencyGetMode Mode)
{
	auto& Cache = GetTagCache(Mode);
	Cache.Reset();
	// Always rely on cache of dependencies and force update of requested tags
	AppendTags_Internal(OUT Cache, Mode, false, true);
}

FGameplayTagContainer& IGameplayTagDependencyInterface::GetTagCache(EGameplayTagDependencyGetMode Mode)
{
	switch (Mode)
	{
	case EGameplayTagDependencyGetMode::OwnTags: return CachedTags_Own;
	case EGameplayTagDependencyGetMode::DependencyTags: return CachedTags_Dependencies;
	}
	return CachedTags_All;
}

const FGameplayTagContainer& IGameplayTagDependencyInterface::GetTagCache(EGameplayTagDependencyGetMode Mode) const
{
	return const_cast<IGameplayTagDependencyInterface*>(this)->GetTagCache(Mode);
}

void IGameplayTagDependencyInterface::AppendTags_Internal(
	FGameplayTagContainer& OutTags,
	EGameplayTagDependencyGetMode Mode,
	bool bUseCache,
	bool bUse2ndLevelCache) const
{
	if (bUseCache)
	{
		OutTags.AppendTags(GetTagCache(Mode));
		return;
	}

	// In "All" case, the caches for own and dependencies count as 2nd level cache
	if (Mode == EGameplayTagDependencyGetMode::AllTags && bUse2ndLevelCache)
	{
		OutTags.AppendTags(CachedTags_Own);
		OutTags.AppendTags(CachedTags_Dependencies);
		return;
	}

	const bool bIncludeDependencyTags = Mode != EGameplayTagDependencyGetMode::OwnTags;
	const bool bIncludeOwnTags = Mode != EGameplayTagDependencyGetMode::DependencyTags;

	if (bIncludeOwnTags)
	{
		AppendOwnTags(OutTags);
	}

	if (bIncludeDependencyTags)
	{
		for (auto Dependency : Dependencies)
		{
			if (Dependency.IsValid())
			{
				Dependency->AppendTags_Internal(
					OUT OutTags,
					EGameplayTagDependencyGetMode::AllTags,
					bUse2ndLevelCache,
					bUse2ndLevelCache);
			}
		}
	}
}

void IGameplayTagDependencyInterface::BroadcastTagsChanged_Recursive(const FGameplayTagContainer& AllTagsBefore)
{
	if (AllTagsBefore != CachedTags_All)
	{
		FGameplayTagDependencyChange Change;
		Change.AllTags = CachedTags_All;
		Change.NewTags = CachedTags_All;
		Change.NewTags.RemoveTags(AllTagsBefore);
		Change.RemovedTags = AllTagsBefore;
		Change.RemovedTags.RemoveTags(CachedTags_All);

		OnTagsChanged.Broadcast(Change);

		for (auto Dependant : ImmediateDependants)
		{
			if (Dependant.IsValid())
			{
				FGameplayTagContainer AllDependantTagsBefore = Dependant->CachedTags_All;
				Dependant->UpdateCachedTags(EGameplayTagDependencyGetMode::DependencyTags);
				Dependant->UpdateCachedTags(EGameplayTagDependencyGetMode::AllTags);
				Dependant->BroadcastTagsChanged_Recursive(AllDependantTagsBefore);
			}
		}
	}
}
