// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/TypedGameplayTagContainer.h"

#include "GameplayTags/TypedGameplayTagSettings.h"

FTypedGameplayTagContainer::FTypedGameplayTagContainer(UStruct& InTypedTagStruct, const FGameplayTagContainer& InTags)
{
	TypedTagName = *InTypedTagStruct.GetName();

	PopulateFilterTags();

	// This constructor should only ever be called with tags that are filtered by
	// TTypedGameplayTagContainerValue<T>::CreateChecked
	// ...so it should be safe to just assign them:
	Tags = InTags;
}

void FTypedGameplayTagContainer::SetTags(const FGameplayTagContainer& InTags)
{
	PopulateFilterTags();
	Tags = InTags.Filter(CachedFilterTags);
}

void FTypedGameplayTagContainer::AddTag(const FGameplayTag& TagToAdd)
{
	PopulateFilterTags();
	if (TagToAdd.MatchesAny(CachedFilterTags))
	{
		Tags.AddTag(TagToAdd);
	}
}

void FTypedGameplayTagContainer::AppendTags(FGameplayTagContainer const& TagsToAppend)
{
	PopulateFilterTags();
	Tags.AppendTags(CachedFilterTags.Filter(TagsToAppend));
}

bool FTypedGameplayTagContainer::RemoveTag(const FGameplayTag& TagToRemove, bool bDeferParentTags)
{
	return Tags.RemoveTag(TagToRemove);
}

void FTypedGameplayTagContainer::RemoveTags(const FGameplayTagContainer& TagsToRemove)
{
	Tags.RemoveTags(TagsToRemove);
}

void FTypedGameplayTagContainer::Reset(int32 Slack)
{
	Tags.Reset(Slack);
}

void FTypedGameplayTagContainer::PopulateFilterTags()
{
	CachedFilterTags.Reset();
	auto& TypedTagSettings = *GetMutableDefault<UTypedGameplayTagSettings>();
	if (TypedTagName.IsNone() == false)
	{
		TypedTagSettings.GetAllRootTags(OUT CachedFilterTags, TypedTagName);
	}
}

void UTypedGameplayTagContainerLibrary::SetTypedContainerTags(
	FTypedGameplayTagContainer& Container,
	const FGameplayTagContainer& Tags)
{
	Container.SetTags(Tags);
}

void UTypedGameplayTagContainerLibrary::AddTagToTypedContainer(
	FTypedGameplayTagContainer& Container,
	const FGameplayTag& TagToAdd)
{
	Container.AddTag(TagToAdd);
}

void UTypedGameplayTagContainerLibrary::AppendTagsToTypedContainer(
	FTypedGameplayTagContainer& Container,
	FGameplayTagContainer const& TagsToAppend)
{
	Container.AppendTags(TagsToAppend);
}

bool UTypedGameplayTagContainerLibrary::RemoveTagFromTypedContainer(
	FTypedGameplayTagContainer& Container,
	const FGameplayTag& TagToRemove)
{
	return Container.RemoveTag(TagToRemove);
}

void UTypedGameplayTagContainerLibrary::RemoveTagsFromTypedContainer(
	FTypedGameplayTagContainer& Container,
	const FGameplayTagContainer& TagsToRemove)
{
	Container.RemoveTags(TagsToRemove);
}

void UTypedGameplayTagContainerLibrary::ResetTypedContainer(FTypedGameplayTagContainer& Container)
{
	Container.Reset();
}
