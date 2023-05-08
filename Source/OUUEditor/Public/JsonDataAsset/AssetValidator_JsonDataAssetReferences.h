// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"

#include "AssetValidator_JsonDataAssetReferences.generated.h"

/**
 * This validator checks that only the FJsonDataAssetPath is used to reference json content.
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
