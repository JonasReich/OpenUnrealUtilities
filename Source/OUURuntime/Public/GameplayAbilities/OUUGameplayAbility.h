// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Abilities/GameplayAbility.h"
#include "CoreMinimal.h"

#include "OUUGameplayAbility.generated.h"

/**
 * Custom gameplay ability that provides friend access to FGameplayDebuggerCategory_OUUAbilities
 * (required to access some of the protected members of the parent class)
 */
UCLASS()
class OUURUNTIME_API UOUUGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
private:
	friend class FGameplayDebuggerCategory_OUUAbilities;
};
