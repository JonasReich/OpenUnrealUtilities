// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Editor/UnrealEdEngine.h"

struct FAssetData;

/**
 * Filter out json data assets from all asset pickers.
 */
class FJsonAssetReferenceFilter : public IAssetReferenceFilter
{
public:
	FJsonAssetReferenceFilter(const FAssetReferenceFilterContext& Context);

	// Pass this asset as "context owner asset" to an asset property entry box to communicate that it should pass
	// the global filter.
	// Intended to communicate when assets are referenced via FJsonDataAssetPath -> then the filter passes.
	// In all other cases, we want the filter to fail.
	static FAssetData PassFilterKey();

	// - IAssetReferenceFilter interface
	bool PassesFilter(const FAssetData& AssetData, FText* OutOptionalFailureReason = nullptr) const override;
	// -- IAssetReferenceFilter
private:
	FAssetReferenceFilterContext Context;
	TSet<FTopLevelAssetPath> JsonDataAssetClassPaths;
};
