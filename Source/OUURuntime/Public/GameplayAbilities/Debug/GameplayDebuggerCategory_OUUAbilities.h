// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "AbilitySystemComponent.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "CoreMinimal.h"
	#include "GameplayDebuggerCategory.h"

class UOUUAbilitySystemComponent;

class AActor;
class APlayerController;
class UCanvas;

/**
 * This is a modified copy of FGameplayDebuggerCategory_Abilities that focuses on improved local debug data
 * by merging in some of the functionality included with "ShowDebug AbilitySystem" command
 * (e.g. ability task states, better layout, etc).
 *
 * To accomplish this and reduce the overhead required by custom mods we chose to skip the multiplayer
 * functionality of the gameplay debugger (for now), so the replication data is not actually replicated
 * and data is gathered directly in the DrawData function.
 *
 * This may be changed later when this implementation becomes a bit more stable.
 */
class OUURUNTIME_API FGameplayDebuggerCategory_OUUAbilities : public FGameplayDebuggerCategory
{
public:
	static auto GetCategoryName() { return TEXT("Abilities (OUU)"); }

	void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

protected:
	struct FAbilitySystemComponentDebugInfo
	{
		FAbilitySystemComponentDebugInfo() { FMemory::Memzero(*this); }

		UCanvas* Canvas;

		bool bPrintToLog;

		float XPos;
		float YPos;
		float OriginalX;
		float OriginalY;
		float MaxY;
		float NewColumnYPadding;
		float YL;

		bool Accumulate;
		TArray<FString> Strings;
	};

	float NumColumns = 4;

	static void DrawBackground(
		FGameplayDebuggerCanvasContext& CanvasContext,
		const FVector2D& BackgroundLocation,
		const FVector2D& BackgroundSize);

	/**
	 * Custom debugging implementation.
	 * Only works for UOUUAbilitySystemComponent, because most debug visualizations need friend access to protected
	 * members. This is the equivalent of UAbilitySystemComponent::Debug_Internal, which is still used for native
	 * UAbilitySystemComponents.
	 */
	void Debug_Custom(FAbilitySystemComponentDebugInfo& Info, UOUUAbilitySystemComponent* AbilitySystem) const;

	static void GetAttributeAggregatorSnapshot(
		UOUUAbilitySystemComponent* AbilitySystem,
		const FGameplayAttribute& Attribute,
		FAggregator SnapshotAggregator);
	void DrawTitle(FAbilitySystemComponentDebugInfo& Info, const FString& DebugTitle) const;
	void DrawDebugHeader(FAbilitySystemComponentDebugInfo& Info, const UOUUAbilitySystemComponent* AbilitySystem) const;

	void AccumulateScreenPos(FAbilitySystemComponentDebugInfo& Info) const;
	void NewColumn(FAbilitySystemComponentDebugInfo& Info) const;
	void NewColumnForCategory_Optional(FAbilitySystemComponentDebugInfo& Info) const;

	void DebugLine(
		FAbilitySystemComponentDebugInfo& Info,
		const FString& Str,
		float XOffset,
		float YOffset,
		int32 MinTextRowsToAdvance = 0) const;

	void AddTagList(
		FAbilitySystemComponentDebugInfo& Info,
		const UOUUAbilitySystemComponent* AbilitySystem,
		FGameplayTagContainer Tags,
		const FString& TagsListTitle) const;
};

#endif // WITH_GAMEPLAY_DEBUGGER
