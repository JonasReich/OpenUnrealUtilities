// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "UObject/WeakInterfacePtr.h"

#include "GameplayTagDependencies.generated.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct FGameplayTagDependencyChange
{
	GENERATED_BODY()
public:
	UPROPERTY()
	FGameplayTagContainer NewTags;
	UPROPERTY()
	FGameplayTagContainer RemovedTags;
	UPROPERTY()
	FGameplayTagContainer AllTags;
};

// Change Change		Change to the tags on this dependency collection
DECLARE_DYNAMIC_DELEGATE_OneParam(FGameplayTagDependencyEvent, const FGameplayTagDependencyChange&, Change);
// Change Change		Change to the tags on this dependency collection
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FGameplayTagDependencyMulticastEvent,
	const FGameplayTagDependencyChange&,
	Change);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UINTERFACE(BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class OUURUNTIME_API UGameplayTagDependencyInterface : public UInterface
{
	GENERATED_BODY()
};

class OUURUNTIME_API IGameplayTagDependencyInterface : public IInterface
{
	GENERATED_BODY()
public:
	friend struct FGameplayTagDependencySource;

	// This appends all tags (own + from dependencies)
	UFUNCTION(BlueprintCallable,Category="OUU|Runtime|GameplayTags")
	virtual void AppendTags(FGameplayTagContainer& OutTags) const;

	// Append tags introduced by this tag container.
	// This is the only function that should be overriden in derived classes.
	UFUNCTION(BlueprintCallable,Category="OUU|Runtime|GameplayTags")
	virtual void AppendOwnTags(FGameplayTagContainer& OutTags) const {}

	UFUNCTION(BlueprintCallable,Category="OUU|Runtime|GameplayTags")
	virtual void BroadcastTagsChanged();

	// Event for external consumers. Not intended for source chaining.
	UFUNCTION(BlueprintCallable,Category="OUU|Runtime|GameplayTags")
	virtual void BindEventToOnTagsChanged(FGameplayTagDependencyEvent Event);

	UFUNCTION(BlueprintCallable,Category="OUU|Runtime|GameplayTags")
	virtual void UnbindEventFromOnTagsChanged(FGameplayTagDependencyEvent Event);

	UFUNCTION(BlueprintCallable,Category="OUU|Runtime|GameplayTags")
	virtual void AddDependency(TScriptInterface<IGameplayTagDependencyInterface> Dependency);

	UFUNCTION(BlueprintCallable,Category="OUU|Runtime|GameplayTags")
	virtual void RemoveDependency(TScriptInterface<IGameplayTagDependencyInterface> Dependency);

	TMap<FGameplayTag, const UObject*> GetImmediateTagSources() const;
	void GetOriginalTagSources(TMap<FGameplayTag, TSet<const UObject*>>& InOutResult) const;

private:
	enum class EGameplayTagDependencyGetMode
	{
		OwnTags,
		DependencyTags,
		AllTags
	};

	TArray<TWeakInterfacePtr<IGameplayTagDependencyInterface>> Dependencies;

	// This split is just internal for faster propagation + debugging purposes.
	// From the outside we only ever care about all tags.
	FGameplayTagContainer CachedTags_Own;
	FGameplayTagContainer CachedTags_Dependencies;
	FGameplayTagContainer CachedTags_All;

	// A little bit like a shared multicast delegate but implemented manually with weak handles to the sources that need
	// a change call.
	TArray<TWeakInterfacePtr<IGameplayTagDependencyInterface>> ImmediateDependants;

	FGameplayTagDependencyMulticastEvent OnTagsChanged;

	void UpdateCachedTags(EGameplayTagDependencyGetMode Mode);
	FGameplayTagContainer& GetTagCache(EGameplayTagDependencyGetMode Mode);
	const FGameplayTagContainer& GetTagCache(EGameplayTagDependencyGetMode Mode) const;
	void AppendTags_Internal(
		FGameplayTagContainer& OutTags,
		EGameplayTagDependencyGetMode Mode,
		bool bUseCache,
		bool bUse2ndLevelCache) const;
	void BroadcastTagsChanged_Recursive(const FGameplayTagContainer& AllTagsBefore);
};
