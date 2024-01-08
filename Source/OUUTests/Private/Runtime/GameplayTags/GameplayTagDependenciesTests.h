// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayTags/GameplayTagDependencies.h"

#include "GameplayTagDependenciesTests.generated.h"

UCLASS(HideDropdown, Hidden)
class UGameplayTagDependency_TestObject : public UObject, public IGameplayTagDependencyInterface
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FGameplayTagContainer SourceContainer;

	void AppendOwnTags(FGameplayTagContainer& OutTags) const override { OutTags.AppendTags(SourceContainer); }
};

UCLASS(HideDropdown, Hidden)
class UGameplayTagDependency_TestEventHandler : public UObject, public IGameplayTagDependencyInterface
{
	GENERATED_BODY()
public:
	FGameplayTagDependencyChange LastChange;
	int32 NumDelegateCalled = 0;
	UFUNCTION()
	void HandleOnTagsChanged(const FGameplayTagDependencyChange& Change)
	{
		LastChange = Change;
		NumDelegateCalled++;
	}
};
