// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"

#include "OUUHardReferenceValidator.generated.h"

// Check that assets labeled with "NoHardReferences" are not hard referenced by any other asset.
UCLASS()
class UOUUHardReferenceValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	// - UEditorValidatorBase
	bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
		override;
	// --
};
