// Copyright (c) 2022 Jonas Reich

#include "GameplayAbilities/OUUAbilitySystemComponent.h"

int32 UOUUAbilitySystemComponent::HandleGameplayEvent(FGameplayTag EventTag, const FGameplayEventData* Payload)
{
	if (Payload != nullptr)
	{
		CircularGameplayEventHistory.Add(FOUUGameplayEventData(*Payload));
	}
	return Super::HandleGameplayEvent(EventTag, Payload);
}
