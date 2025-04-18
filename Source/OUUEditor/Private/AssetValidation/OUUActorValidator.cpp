// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUActorValidator.h"

#include "Misc/DataValidation.h"

#if UE_VERSION_OLDER_THAN(5, 4, 0)
bool UOUUActorValidator::CanValidateAsset_Implementation(UObject* InAsset) const
#else
bool UOUUActorValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& InContext) const
#endif
{
	if (IsValid(InAsset) == false)
	{
		return false;
	}

	return InAsset->IsA<AActor>();
}

#if UE_VERSION_OLDER_THAN(5, 4, 0)
EDataValidationResult UOUUActorValidator::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
#else
EDataValidationResult UOUUActorValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& Context)
#endif
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	auto* Actor = Cast<AActor>(InAsset);

	if (IsValid(Actor) == false)
	{
#if UE_VERSION_OLDER_THAN(5, 4, 0)
		AssetFails(InAsset, INVTEXT("Asset is not an actor"), IN OUT ValidationErrors);
#else
		Context.AddError(INVTEXT("Asset is not an actor"));
#endif
		return EDataValidationResult::Invalid;
	}

	USceneComponent* RootComponent = Actor->GetRootComponent();
	TArray<USceneComponent*> AdditionalRoots;
	Actor->ForEachComponent<USceneComponent>(false, [&](const USceneComponent* SceneComponent) {
		auto* AttachRoot = SceneComponent->GetAttachmentRoot();
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

			auto WarningMessage = FText::Format(
				INVTEXT("Actor has multiple root components: {0} and {1}"),
				FText::FromName(RootComponent->GetFName()),
				FText::FromName(AttachRoot->GetFName()));
#if UE_VERSION_OLDER_THAN(5, 4, 0)
			AssetWarning(Actor, WarningMessage);
#else
			Context.AddWarning(WarningMessage);
#endif
		}
	});

#if UE_VERSION_OLDER_THAN(5, 4, 0)
	if (Result == EDataValidationResult::Valid)
	{
		AssetPasses(InAsset);
	}
#endif
	return Result;
}
