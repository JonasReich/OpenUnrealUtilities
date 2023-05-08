// Copyright (c) 2022 Jonas Reich

#include "JsonDataAsset/JsonAssetReferenceFilter.h"

#include "JsonDataAsset/JsonDataAsset.h"
#include "Templates/ArrayUtils.h"

FJsonAssetReferenceFilter::FJsonAssetReferenceFilter(const FAssetReferenceFilterContext& Context)
{
	this->Context = Context;
}

FAssetData FJsonAssetReferenceFilter::PassFilterKey()
{
	// Maybe find something better than the CDO?
	// (e.g. a completely fake object)

	// UJsonDataAsset::StaticClass()->GetDefaultObject()

	// Fake asset data to fullfil the requirements for context data.
	return FAssetData(
		TEXT("/Script/OUU"),
		TEXT("/Script/OUU.JsonData"),
		FTopLevelAssetPath(TEXT("/Script/OUU.JsonData")));
}

bool FJsonAssetReferenceFilter::PassesFilter(const FAssetData& AssetData, FText* OutOptionalFailureReason) const
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
