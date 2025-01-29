// Copyright (c) 2024 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "Misc/EngineVersionComparison.h"

#include "OUUTextValidator.generated.h"

// Validates text properties in assets not to have localized texts (as configured in UOUUAssetValidationSettings).
UCLASS()
class UOUUTextValidator : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
	// - UEditorValidatorBase
	bool CanValidateAsset_Implementation(UObject* InAsset) const override;

#if UE_VERSION_OLDER_THAN(5, 4, 0)
	EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& OutValidationErrors)
		override;
#else
	EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& Context) override;
#endif
	// --
};
