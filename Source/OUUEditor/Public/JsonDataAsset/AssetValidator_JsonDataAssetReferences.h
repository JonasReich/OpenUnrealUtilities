// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"

#include "AssetValidator_JsonDataAssetReferences.generated.h"

/**
 * This validator checks that only the FJsonDataAssetPath is used to reference json content (this includes the smart
 * pointer types that are based on FJsonDataAssetPath).
 */
UCLASS()
class UAssetValidator_JsonDataAssetReferences : public UEditorValidatorBase
{
	GENERATED_BODY()

protected:
	// - UEditorValidatorBase
	bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
		override;
	// --

private:
	bool HasJsonDependency(UObject* InAsset);

	// The _actual_ implementation
	EDataValidationResult ValidateLoadedAsset_Impl(UObject* InAsset, TArray<FText>& ValidationErrors);
};
