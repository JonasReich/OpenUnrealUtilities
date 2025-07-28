// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "OUUAssetValidatorBase_EngineVersionGlue.h"

#include "OUUTextValidator.generated.h"

// Validates text properties in assets not to have localized texts (as configured in UOUUAssetValidationSettings).
UCLASS()
class UOUUTextValidator : public UOUUAssetValidatorBase_EngineVersionGlue
{
	GENERATED_BODY()
public:	// - UEditorValidatorBase / UOUUAssetValidatorBase_EngineVersionGlue (depending on engine version)
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
