// Copyright (c) 2021 Jonas Reich

#pragma once
#include "AbilitySystemComponent.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "CoreMinimal.h"
#include "GameplayDebuggerCategory.h"

class UOUUAbilitySystemComponent;

class AActor;
class APlayerController;

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
	static auto GetCategoryName()
	{
		return TEXT("Abilities (OUU)");
	}

	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

protected:
	struct FAbilitySystemComponentDebugInfo
	{
		FAbilitySystemComponentDebugInfo()
		{
			FMemory::Memzero(*this);
		}

		class UCanvas* Canvas;

		bool bPrintToLog;

		float XPos;
		float YPos;
		float OriginalX;
		float OriginalY;
		float MaxY;
		float NewColumnYPadding;
		float YL;

		bool Accumulate;
		TArray<FString>	Strings;
	};

	float NumColumns = 4;

	void DrawBackground(FGameplayDebuggerCanvasContext& CanvasContext, const FVector2D& BackgroundLocation, const FVector2D& BackgroundSize);

	/**
	 * Custom debugging implementation.
	 * Only works for UOUUAbilitySystemComponent, because most debug visualizations need friend access to protected members.
	 * This is the equivalent of UAbilitySystemComponent::Debug_Internal, which is still used for native UAbilitySystemComponents.
	 */
	void Debug_Custom(FAbilitySystemComponentDebugInfo& Info, UOUUAbilitySystemComponent* AbilitySystem);

	static void GetAttributeAggregatorSnapshot(UOUUAbilitySystemComponent* AbilitySystem, FGameplayAttribute& Attribute, FAggregator SnaphotAggregator);
	void DrawTitle(FAbilitySystemComponentDebugInfo& Info, FString DebugTitle);
	void DrawDebugHeader(FAbilitySystemComponentDebugInfo& Info, UOUUAbilitySystemComponent* AbilitySystem);

	void AccumulateScreenPos(FAbilitySystemComponentDebugInfo& Info);
	void NewColumn(FAbilitySystemComponentDebugInfo& Info) const;
	void NewColumnForCategory_Optional(FAbilitySystemComponentDebugInfo& Info) const;

	void DebugLine(FAbilitySystemComponentDebugInfo& Info, FString Str, float XOffset, float YOffset, int32 MinTextRowsToAdvance = 0);

	void AddTagList(FAbilitySystemComponentDebugInfo& Info, UOUUAbilitySystemComponent* AbilitySystem, FGameplayTagContainer Tags, FString TagsListTitle);
};

#endif // WITH_GAMEPLAY_DEBUGGER
