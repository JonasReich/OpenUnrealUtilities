// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "AbilitySystemComponent.h"
#include "GameplayAbilities/OUUAbilitySystemComponent.h"
#include "GameplayAbilities/OUUGameplayAbility.h"
#include "GameplayCueManager.h"

#if WITH_GAMEPLAY_DEBUGGER

	#include "CoreMinimal.h"
	#include "GameplayDebuggerCategory.h"

class UOUUAbilitySystemComponent;

class AActor;
class APlayerController;
class UCanvas;

/**
 * This is a heavily modified copy of FGameplayDebuggerCategory_Abilities that focuses on improved local debug data
 * by merging in some of the functionality included with "ShowDebug AbilitySystem" command
 * (e.g. ability task states, better layout, etc).
 *
 * To accomplish this and reduce the overhead required by custom mods we chose to skip the multiplayer
 * functionality of the gameplay debugger.
 */
class OUURUNTIME_API FGameplayDebuggerCategory_OUUAbilities : public FGameplayDebuggerCategory
{
public:
	static auto GetCategoryName() { return TEXT("Abilities (OUU)"); }

	void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;
	void DebugDrawGameplayEffectModifier(
		FActiveGameplayEffect& ActiveGE,
		const FModifierSpec& ModSpec,
		const FGameplayModifierInfo& ModInfo);

protected:
	struct FAbilitySystemComponentDebugInfo
	{
		float XPos = 0.f;
		float YPos = 0.f;

		float MinX = 0.f;
		float MaxY = 0.f;
		float MinY = 0.f;
		float LineHeight = 0.f;

		TArray<FString> Strings;
	} DebugInfo;

	UCanvas* Canvas = nullptr;
	UOUUAbilitySystemComponent* AbilitySystem = nullptr;

	void DrawGameplayEffect(FActiveGameplayEffect& ActiveGE);

	void DrawGameplayAbilityInstance(UOUUGameplayAbility* Instance);
	void DetermineAbilityStatusText(
		const FGameplayTagContainer& BlockedAbilityTags,
		const FGameplayAbilitySpec& AbilitySpec,
		const UGameplayAbility* Ability,
		FString& OutStatusText,
		FColor& OutAbilityTextColor) const;
	void DrawAbility(const FGameplayTagContainer& BlockedAbilityTags, const FGameplayAbilitySpec& AbilitySpec);

	void DrawGameplayCue(
		UGameplayCueManager* CueManager,
		FString BaseCueTagString,
		UGameplayCueSet* CueSet,
		FGameplayTag ThisGameplayCueTag);

	void DrawAttribute(FGameplayAttribute& Attribute, bool ColorSwitch);

	static void DrawBackground(
		FGameplayDebuggerCanvasContext& CanvasContext,
		const FVector2D& BackgroundLocation,
		const FVector2D& BackgroundSize);

	void DrawDebugBody();

	void DrawTitle(const FString& DebugTitle);
	void DrawDebugHeader();

	void AccumulateScreenPos();
	void NewColumn();
	void NewColumnForCategory_Optional();

	void DebugLine(
		const FString& Str,
		float XOffset,
		int32 MinTextRowsToAdvance);

	void AddTagList(
		FGameplayTagContainer Tags,
		const FString& TagsListTitle);
};

#endif // WITH_GAMEPLAY_DEBUGGER
