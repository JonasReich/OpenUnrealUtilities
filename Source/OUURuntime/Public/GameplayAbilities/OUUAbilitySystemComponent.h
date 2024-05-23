// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "AbilitySystemComponent.h"
#include "Templates/CircularArrayAdaptor.h"

#include "OUUAbilitySystemComponent.generated.h"

/**
 * Utility structure used to copy data from FGameplayEventData for long time storage in an event history.
 * The original structure from the engine is very likely to cause crashes during copy assignment.
 */
USTRUCT(BlueprintType)
struct OUURUNTIME_API FOUUGameplayEventData
{
	GENERATED_BODY()
public:
	FOUUGameplayEventData() = default;
	FOUUGameplayEventData(int32 EventCounter, const FGameplayEventData& SourcePayload) :
		EventNumber(EventCounter),
		Timestamp(FDateTime::Now()),
		EventTag(SourcePayload.EventTag),
		Instigator(SourcePayload.Instigator),
		Target(SourcePayload.Target),
		// InstigatorTags(SourcePayload.InstigatorTags),
		// TargetTags(SourcePayload.TargetTags),
		EventMagnitude(SourcePayload.EventMagnitude)
	{
	}

	UPROPERTY()
	int32 EventNumber = 0;

	/** When the event occured (compare with FDateTime::Now()) */
	UPROPERTY()
	FDateTime Timestamp = FDateTime::MinValue();

	/** Tag of the event that triggered this */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	FGameplayTag EventTag = {};

	/** The instigator of the event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	const AActor* Instigator = nullptr;

	/** The target of the event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	const AActor* Target = nullptr;

	// #TODO figure out why the gameplay tag containers cause crashes when reallocating for circular event queue
	/** Tags that the instigator has */
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	// FGameplayTagContainer InstigatorTags;

	/** Tags that the target has */
	// UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	// FGameplayTagContainer TargetTags;

	/** The magnitude of the triggering event */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameplayAbilityTriggerPayload)
	float EventMagnitude = 0.f;
};

/**
 * Custom ability system component that provides friend access to FGameplayDebuggerCategory_OUUAbilities
 * (required to access some of the protected members of the parent class)
 * and record additional debugging data, e.g. history of gameplay events.
 */
UCLASS()
class OUURUNTIME_API UOUUAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
private:
	friend class FGameplayDebuggerCategory_OUUAbilities;

public:
	// - UAbilitySystemComponent
	int32 HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload) override;
	// --

protected:
	int32 EventCounter = 0;

	/**
	 * History of all gameplay events encountered via HandleGameplayEvent()
	 * Intended for debugging purposes.
	 */
	UPROPERTY(Transient)
	TArray<FOUUGameplayEventData> GameplayEventHistory;

	/**
	 * Circular buffer adapter for the gameplay event history.
	 * Use this to access the history elements!
	 */
	TCircularArrayAdaptor<FOUUGameplayEventData> CircularGameplayEventHistory{GameplayEventHistory, 50};
};
