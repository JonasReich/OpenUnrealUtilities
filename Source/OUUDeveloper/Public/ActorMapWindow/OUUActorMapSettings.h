// Copyright (c) 2026 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "ActorMapWindow/OUUActorMapQuery.h"
#include "Engine/DeveloperSettings.h"

#include "OUUActorMapSettings.generated.h"

UCLASS(DefaultConfig, Config = Game)
class OUUDEVELOPER_API UOUUActorMapSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	UPROPERTY(Config, EditAnywhere)
	TArray<FOUUActorMapQuery> DefaultQueries;
};
