// Copyright (c) 2026 Jonas Reich & Contributors

#include "Misc/OUUAssetRegistryUtils.h"

#include "AssetRegistry/IAssetRegistry.h"

namespace OUU::Runtime::AssetRegistryUtils
{
	void WaitForAssetRegistry()
	{
		IAssetRegistry& AssetRegistry = IAssetRegistry::GetChecked();
		if (AssetRegistry.IsSearchAsync() && AssetRegistry.IsSearchAllAssets())
		{
			AssetRegistry.WaitForCompletion();
		}
		else
		{
			AssetRegistry.SearchAllAssets(true /* bSynchronousSearch */);
		}
	}
} // namespace OUU::Runtime::AssetRegistryUtils
