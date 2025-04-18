// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "Misc/EngineVersionComparison.h"

#include "OUUBlueprintValidator.generated.h"

// Validates blueprints (esp actor blueprints) with some generic checks.
UCLASS()
class UOUUBlueprintValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
// - UEditorValidatorBase
#if UE_VERSION_OLDER_THAN(5, 4, 0)
	bool CanValidateAsset_Implementation(UObject* InAsset) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
		override;
#else
	bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InObject,
		FDataValidationContext& InContext) const override;
	EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& Context) override;
#endif
	// --
};
