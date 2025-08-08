// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "OUUAssetValidatorBase_EngineVersionGlue.h"

#include "OUUBlueprintValidator.generated.h"

// Validates blueprints (esp actor blueprints) with some generic checks.
UCLASS()
class UOUUBlueprintValidator : public UOUUAssetValidatorBase_EngineVersionGlue
{
	GENERATED_BODY()
public:
	// - UEditorValidatorBase / UOUUAssetValidatorBase_EngineVersionGlue (depending on engine version)
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
