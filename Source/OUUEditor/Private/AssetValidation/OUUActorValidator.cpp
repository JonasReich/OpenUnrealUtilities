// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUActorValidator.h"

bool UOUUActorValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	if (IsValid(InAsset) == false)
	{
		return false;
	}

	return InAsset->IsA<AActor>();
}

EDataValidationResult UOUUActorValidator::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	auto* Actor = Cast<AActor>(InAsset);

	if (IsValid(Actor) == false)
	{
		AssetFails(InAsset, INVTEXT("Asset is not an actor"), IN OUT ValidationErrors);
		return EDataValidationResult::Invalid;
	}

	USceneComponent* RootComponent = Actor->GetRootComponent();
	TArray<USceneComponent*> AdditionalRoots;
	Actor->ForEachComponent<USceneComponent>(false, [&](const USceneComponent* SceneComponent) {
		auto* AttachRoot = SceneComponent->GetAttachmentRoot();
		if (AttachRoot->GetOwner() == Actor && AttachRoot != RootComponent)
		{
			// Search class, so we don't need the module dependency just for this class check
			auto* NavigationDataClass =
				UClass::TryFindTypeSlow<UClass>(TEXT("NavigationData"), EFindFirstObjectOptions::EnsureIfAmbiguous);

			// Ignore the recast actor, which has 2 roots for some reason
			if (Actor->IsA(NavigationDataClass))
			{
				return;
			}

			AssetWarning(
				Actor,
				FText::Format(
					INVTEXT("Actor has multiple root components: {0} and {1}"),
					FText::FromName(RootComponent->GetFName()),
					FText::FromName(AttachRoot->GetFName())));
		}
	});

	if (Result == EDataValidationResult::Valid)
	{
		AssetPasses(InAsset);
	}
	return Result;
}
