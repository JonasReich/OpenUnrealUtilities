// Copyright (c) 2022 Jonas Reich

#include "GameplayAbilities/OUUAbilitySystemComponent.h"

int32 UOUUAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (Payload != nullptr)
	{
		EventCounter++;
		CircularGameplayEventHistory.Add(FOUUGameplayEventData(EventCounter, *Payload));
	}
	return Super::HandleGameplayEvent(EventTag, Payload);
}
