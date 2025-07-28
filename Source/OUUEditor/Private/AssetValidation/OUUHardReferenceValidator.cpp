// Copyright (c) 2025 Jonas Reich & Contributors

#include "AssetValidation/OUUHardReferenceValidator.h"

#include "AssetRegistry/IAssetRegistry.h"

bool UOUUHardReferenceValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	return true;
}

EDataValidationResult UOUUHardReferenceValidator::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	FAssetData TargetAsset{InAsset};
	FAssetIdentifier TargetAssetId{*InAsset->GetPackage()->GetPathName()};

	auto& AssetRegistry = *IAssetRegistry::Get();

	TArray<FAssetIdentifier> Dependencies;
	AssetRegistry.GetDependencies(
		TargetAssetId,
		OUT Dependencies,
		UE::AssetRegistry::EDependencyCategory::Package,
		UE::AssetRegistry::FDependencyQuery(UE::AssetRegistry::EDependencyQuery::Hard));

	TArray<FAssetData> DependencyAssets;
	FName TagValue;
	for (auto& Dependency : Dependencies)
	{
		DependencyAssets.Reset();
		AssetRegistry.GetAssetsByPackageName(Dependency.PackageName, OUT DependencyAssets);
		for (auto& DepAsset : DependencyAssets)
		{
			if (DepAsset.GetTagValue(TEXT("NotHardReferencable"), OUT TagValue) && TagValue == TEXT("true"))
			{
				auto ErrorMessage = FText::Format(
					INVTEXT("Hard reference to asset marked \"NotHardReferencable\": {0}"),
					FText::FromName(Dependency.PackageName));
				AssetFails(InAsset, ErrorMessage, ValidationErrors);
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	if (Result != EDataValidationResult::Invalid)
	{
		AssetPasses(InAsset);
	}
	return Result;
}
