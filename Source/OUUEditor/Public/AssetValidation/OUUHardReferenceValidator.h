// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "OUUAssetValidatorBase_EngineVersionGlue.h"

#include "OUUHardReferenceValidator.generated.h"

// Check that assets labeled with "NoHardReferences" are not hard referenced by any other asset.
UCLASS()
class UOUUHardReferenceValidator : public UOUUAssetValidatorBase_EngineVersionGlue
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
