// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Engine/DeveloperSettings.h"

#include "OUUAssetValidationSettings.generated.h"

UCLASS(Config = Editor, DefaultConfig)
class UOUUAssetValidationSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	FORCEINLINE static const UOUUAssetValidationSettings& Get() { return *GetDefault<UOUUAssetValidationSettings>(); }

	// Assets of these classes must not have any properties with any localized FTexts.
	// FTexts from string tables and culture invariants are permitted.
	UPROPERTY(Config, EditDefaultsOnly)
	TArray<TSoftClassPtr<UObject>> ValidateNoLocalizedTextsClasses;

	// Assets of these classes are allowed to have non-localized FTexts even if they match the
	// ValidateNoLocalizedTextsClasses above.
	UPROPERTY(Config, EditDefaultsOnly)
	TArray<TSoftClassPtr<UObject>> IgnoreNoLocalizedTextsClasses;
};
