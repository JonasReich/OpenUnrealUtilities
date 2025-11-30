// Copyright (c) 2025 Jonas Reich & Contributors

#include "AssetValidation/OUUHardReferenceValidator.h"

#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/DataValidation.h"

bool UOUUHardReferenceValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InObject,
	FDataValidationContext& InContext) const
{
	// do not run this validator on actors -
	// the dependency search via asset registry iterates and loads far too many packages
	return IsValid(InObject) && InObject->IsA<AActor>() == false;
}

EDataValidationResult UOUUHardReferenceValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& Context)
{
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

				Context.AddError(ErrorMessage);
				return EDataValidationResult::Invalid;
			}
		}
	}
	return EDataValidationResult::Valid;
}
