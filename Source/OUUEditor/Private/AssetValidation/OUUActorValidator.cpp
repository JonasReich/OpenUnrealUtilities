// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUActorValidator.h"

#include "Misc/DataValidation.h"

bool UOUUActorValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& InContext) const
{
	return IsValid(InAsset) && InAsset->IsA<AActor>();
}

EDataValidationResult UOUUActorValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& Context)
{
	const auto* Actor = Cast<AActor>(InAsset);

	if (IsValid(Actor) == false)
	{
		Context.AddError(INVTEXT("Asset is not an actor"));
		return EDataValidationResult::Invalid;
	}

	const USceneComponent* RootComponent = Actor->GetRootComponent();
	TArray<USceneComponent*> AdditionalRoots;
	Actor->ForEachComponent<USceneComponent>(false, [&](const USceneComponent* SceneComponent) {
		const auto* AttachRoot = SceneComponent->GetAttachmentRoot();
		if (AttachRoot->GetOwner() == Actor && AttachRoot != RootComponent)
		{
			// Search class, so we don't need the module dependency just for this class check
			if (NavigationDataClass == nullptr)
			{
				NavigationDataClass = UClass::TryFindTypeSlow<UClass>(
					TEXT("/Script/NavigationSystem.NavigationData"),
					EFindFirstObjectOptions::EnsureIfAmbiguous);
			}

			// Ignore the recast actor, which has 2 roots for some reason
			if (Actor->IsA(NavigationDataClass))
			{
				return;
			}

			const auto WarningMessage = FText::Format(
				INVTEXT("Actor has multiple root components: {0} and {1}"),
				FText::FromName(RootComponent->GetFName()),
				FText::FromName(AttachRoot->GetFName()));

			Context.AddWarning(WarningMessage);
		}
	});
	return EDataValidationResult::Valid;
}
