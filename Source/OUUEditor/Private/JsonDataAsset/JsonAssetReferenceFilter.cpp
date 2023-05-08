// Copyright (c) 2023 Jonas Reich & Contributors

#include "JsonDataAsset/JsonAssetReferenceFilter.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "JsonDataAsset/JsonDataAsset.h"
#include "Templates/ArrayUtils.h"

FJsonAssetReferenceFilter::FJsonAssetReferenceFilter(const FAssetReferenceFilterContext& Context)
{
	this->Context = Context;

	IAssetRegistry::Get()->GetDerivedClassNames(
		TArray<FTopLevelAssetPath>{FTopLevelAssetPath(TEXT("/Script/OUURuntime"), TEXT("JsonDataAsset"))},
		TSet<FTopLevelAssetPath>{},
		JsonDataAssetClassPaths);
}

FAssetData FJsonAssetReferenceFilter::PassFilterKey()
{
	// Fake asset data to fullfil the requirements for context data.
	return FAssetData(
		TEXT("/Script/OUU"),
		TEXT("/Script/OUU.JsonData"),
		FTopLevelAssetPath(TEXT("/Script/OUU.JsonData")));
}

bool FJsonAssetReferenceFilter::PassesFilter(const FAssetData& AssetData, FText* OutOptionalFailureReason) const
{
	if (Context.ReferencingAssets.Num() == 0)
	{
		// Always pass if we don't know what is referencing the asset.
		// This is specifically required for open asset window (Alt+Shift+O).
		// In some cases this might be a bit to lax, but in those cases we trust on the global
		// UAssetValidator_JsonDataAssetReferences.
		return true;
	}

	if (AssetData.AssetClassPath.IsValid() && JsonDataAssetClassPaths.Contains(AssetData.AssetClassPath))
	{
		if (Context.ReferencingAssets.Contains(PassFilterKey()))
		{
			return true;
		}

		if (OutOptionalFailureReason)
		{
			*OutOptionalFailureReason = INVTEXT("JsonDataAssets may not be referenced directly via object properties. "
												"Use FJsonDataAssetPath instead.");
		}
		return false;
	}

	return true;
}
