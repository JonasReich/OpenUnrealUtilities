// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayTags/OUUGameplayTagLibrary.h"

#include "GameplayTagsManager.h"

FGameplayTag UOUUGameplayTagLibrary::GetParentTag(const FGameplayTag& Tag)
{
	return UGameplayTagsManager::Get().RequestGameplayTagDirectParent(Tag);
}

FGameplayTagContainer UOUUGameplayTagLibrary::GetChildTags(const FGameplayTag& Tag)
{
	return UGameplayTagsManager::Get().RequestGameplayTagChildren(Tag);
}

int32 UOUUGameplayTagLibrary::GetTagDepth(const FGameplayTag& Tag)
{
	return GetTagComponents(Tag).Num();
}

FGameplayTag UOUUGameplayTagLibrary::GetTagUntilDepth(const FGameplayTag& Tag, int32 Depth)
{
	TArray<FName> Names = GetTagComponents(Tag);
	if (Depth > Names.Num())
	{
		return FGameplayTag::EmptyTag;
	}
	Names.SetNum(Depth, true);
	return CreateTagFromComponents(Names);
}

TArray<FName> UOUUGameplayTagLibrary::GetTagComponents(const FGameplayTag& Tag)
{
	TArray<FName> Names;
	UGameplayTagsManager::Get().SplitGameplayTagFName(Tag, OUT Names);
	return Names;
}

FGameplayTag UOUUGameplayTagLibrary::CreateTagFromComponents(const TArray<FName>& TagComponents)
{
	auto TagString =
		FString::JoinBy(TagComponents, TEXT("."), [](const FName& Name) -> FString { return Name.ToString(); });
	return UGameplayTagsManager::Get().RequestGameplayTag(*TagString, false);
}
