// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"

#include "OUURestrictedNamesValidator.generated.h"

// Validates all assets to check if they use any restricted names not enforced by the engine.
UCLASS()
class UOUURestrictedNamesValidator : public UEditorValidatorBase
{
	GENERATED_BODY()

	// - UEditorValidatorBase
public:
	bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InObject,
		FDataValidationContext& InContext) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& Context) override;
	// --
};
