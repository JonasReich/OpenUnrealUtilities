// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameplayTags/GameplayTagQueryParser.h"

#include "OUUActorMapQuery.generated.h"

class FWorldPartitionActorDescInstance;

/**
 * Utility struct that allows querying actors matching certain filter conditions.
 * Conditions are cumulative: All conditions must match for an actor to be included.
 */
USTRUCT(MinimalAPI)
struct FOUUActorMapQuery
{
public:
	GENERATED_BODY()

	/** Color in which the query results are displayed. */
	UPROPERTY(EditAnywhere)
	FColor QueryColor;

	/** String that must be contained within the actor name. Ignored if empty. */
	UPROPERTY(EditAnywhere)
	FString NameFilter;

	/** Regex pattern that actor names must match. Ignored if empty. */
	UPROPERTY(EditAnywhere)
	FString NameRegexPattern;

	/**
	 * Exact name of the actor class or any of its parent classes.
	 * The name must be an exact match, e.g. StaticMeshActor for AStaticMeshActors
	 */
	UPROPERTY(EditAnywhere)
	FString ActorClassName;
	mutable UClass* ResolvedActorClass = nullptr;
	mutable bool bClassNotResolved = true;

	/**
	 * Exact name of a component class on the actor or any of its parent classes.
	 * The name must be an exact match, e.g. StaticMeshComponent for UStaticMeshComponent
	 */
	UPROPERTY(EditAnywhere)
	FString ComponentClassName;

	/**
	 * If this is valid, actors are expected to have a gameplay ability system component
	 * of which the owned gameplay tags are compared with this query.
	 */
	UPROPERTY(EditAnywhere)
	FGameplayTagQuery ActorTagQuery;

	TArray<TTuple<FVector, FString>>* CachedResult = nullptr;

	bool MatchesActor(const AActor* Actor) const;

#if WITH_EDITOR
	bool MatchesActorDescription(const FWorldPartitionActorDescInstance& _ActorDesc) const;
#endif
};