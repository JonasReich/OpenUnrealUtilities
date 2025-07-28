// Copyright (c) 2025 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "EditorValidatorBase.h"
#include "Misc/DataValidation.h"
#include "Misc/EngineVersionComparison.h"

#include "OUUAssetValidatorBase_EngineVersionGlue.generated.h"

// The editor validator base class in this file is just an intermediate class to access some of the functionality
// required to for UE5.4 asset validators in UE5.2 and UE5.3 and make this plugin compile cross engine versions.
// This allows writing UE5.4 style validators using the FDataValidationContext already in 5.2 - with the caveat that the
// validation context does not provide proper metadata (validation use case).
UCLASS(Abstract)
class UOUUAssetValidatorBase_EngineVersionGlue : public UEditorValidatorBase
{
	GENERATED_BODY()
public:
#if UE_VERSION_OLDER_THAN(5, 4, 0)
	bool CanValidateAsset_Implementation(UObject* InAsset) const final override
	{
		FAssetData InAssetData(InAsset);
		FDataValidationContext Context;
		auto Result = CanValidateAsset_Implementation(InAssetData, InAsset, IN OUT Context);
		return Result;
	}
	virtual bool CanValidateAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InObject,
		FDataValidationContext& InContext) const
	{
		return false;
	}

	EDataValidationResult ValidateLoadedAsset_Implementation(UObject* InAsset, TArray<FText>& ValidationErrors)
		final override
	{
		FAssetData InAssetData(InAsset);
		FDataValidationContext Context;
		auto Result = ValidateLoadedAsset_Implementation(InAssetData, InAsset, Context);
		for (auto& Issue : Context.GetIssues())
		{
			if (Issue.Severity <= EMessageSeverity::Error)
			{
				AssetFails(InAsset, Issue.Message, IN OUT ValidationErrors);
			}
			else
			{
				AssetWarning(InAsset, Issue.Message);
			}
		}

		if (Context.GetNumErrors() > 0)
		{
			Result = EDataValidationResult::Invalid;
		}
		else
		{
			AssetPasses(InAsset);
		}

		return Result;
	}

	virtual EDataValidationResult ValidateLoadedAsset_Implementation(
		const FAssetData& InAssetData,
		UObject* InAsset,
		FDataValidationContext& Context)
	{
		return EDataValidationResult::NotValidated;
	}
#endif
};
