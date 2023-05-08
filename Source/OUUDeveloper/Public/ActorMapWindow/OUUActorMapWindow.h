// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/Actor.h"
#include "GameplayTags/GameplayTagQueryParser.h"

namespace OUU::Developer::ActorMapWindow
{
	extern OUUDEVELOPER_API FName GTabName;
	extern OUUDEVELOPER_API FText GTabTitle;

	void OUUDEVELOPER_API RegisterNomadTabSpawner();
	void OUUDEVELOPER_API UnregisterNomadTabSpawner();
	void OUUDEVELOPER_API TryInvokeTab();

	/**
	 * Utility class that allows querying actors matching certain filter conditions.
	 * Conditions are cumulative: All conditions must match for an actor to be included.
	 */
	class OUUDEVELOPER_API FActorQuery
	{
	public:
		struct FResult
		{
		public:
			TArray<AActor*> Actors;
		};

		/** Color in which the query results are displayed. */
		FColor QueryColor;

		/** String that must be contained within the actor name. Ignored if empty. */
		FString NameFilter;

		/** Regex pattern that actor names must match. Ignored if empty. */
		FString NameRegexPattern;

		/**
		 * Exact name of the actor class or any of its parent classes.
		 * The name must be an exact match, e.g. StaticMeshActor for AStaticMeshActors
		 */
		FString ActorClassName;

		/**
		 * If this is valid, actors are expected to have a gameplay ability system component
		 * of which the owned gameplay tags are compared with this query.
		 */
		FGameplayTagQuery ActorTagQuery;

		/**
		 * Cached result from executing the query via ExecuteAndCacheQuery()
		 */
		FResult CachedQueryResult;

		bool MatchesActor(const AActor* Actor) const;

		bool MatchesActorClassSearchString(const AActor* Actor) const;

		FResult ExecuteQuery(UWorld* World) const;

		FORCEINLINE FResult& ExecuteAndCacheQuery(UWorld* World)
		{
			CachedQueryResult = ExecuteQuery(World);
			return CachedQueryResult;
		}
	};
} // namespace OUU::Developer::ActorMapWindow
