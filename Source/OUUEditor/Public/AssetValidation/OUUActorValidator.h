// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"

#include "OUUActorValidator.generated.h"

// Validates actor instances with some generic checks.
UCLASS()
class UOUUActorValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	// - UEditorValidatorBase
	bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
		override;
	// --
};
