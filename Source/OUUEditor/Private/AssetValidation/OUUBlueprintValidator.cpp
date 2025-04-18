// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUBlueprintValidator.h"

#include "Engine/SCS_Node.h"
#include "Misc/DataValidation.h"

enum class EBlueprintHasDefaultRoot
{
	No,
	YesMovable,
	YesNonMovable
};

EBlueprintHasDefaultRoot BlueprintHasNonMovableDefaultRoot(const UBlueprint& Blueprint)
{
	TArray<USCS_Node*> AllNodes = Blueprint.SimpleConstructionScript->GetAllNodes();
	const USCS_Node* RootNode = AllNodes.Num() > 0 ? AllNodes[0] : nullptr;
	const bool bHasDefaultRoot = RootNode == Blueprint.SimpleConstructionScript->GetDefaultSceneRootNode();
	if (bHasDefaultRoot == false)
	{
		return EBlueprintHasDefaultRoot::No;
	}

	const auto* Template = CastChecked<USceneComponent>(RootNode->ComponentTemplate.Get());
	return (Template->Mobility == EComponentMobility::Movable) ? EBlueprintHasDefaultRoot::YesMovable
															   : EBlueprintHasDefaultRoot::YesNonMovable;
}

#if UE_VERSION_OLDER_THAN(5, 4, 0)
bool UOUUBlueprintValidator::CanValidateAsset_Implementation(UObject* InAsset) const
#else
bool UOUUBlueprintValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& InContext) const
#endif
{
	// For now: do not run this validator in cook.
	if (IsValid(InAsset) == false)
	{
		return false;
	}

	return InAsset->IsA<UBlueprint>();
}

#if UE_VERSION_OLDER_THAN(5, 4, 0)
EDataValidationResult UOUUBlueprintValidator::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
#else
EDataValidationResult UOUUBlueprintValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& Context)
#endif
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);

	if (IsValid(Blueprint) == false)
	{
#if UE_VERSION_OLDER_THAN(5, 4, 0)
		AssetFails(InAsset, INVTEXT("Asset is not a blueprint"), IN OUT ValidationErrors);
#else
		Context.AddError(INVTEXT("Asset is not a blueprint"));
#endif

		return EDataValidationResult::Invalid;
	}

	if (Blueprint->ParentClass->IsChildOf<AActor>())
	{
		UClass* ClassToCheck = Blueprint->GeneratedClass;

		while (ClassToCheck->IsInBlueprint())
		{
			TArray<UClass*> ChildClasses;
			GetDerivedClasses(ClassToCheck, ChildClasses);

			const auto* BlueprintToCheck = CastChecked<UBlueprint>(ClassToCheck->ClassGeneratedBy);
			if (ChildClasses.Num() > 0)
			{
				auto Status = BlueprintHasNonMovableDefaultRoot(*BlueprintToCheck);

				if (Status == EBlueprintHasDefaultRoot::YesMovable)
				{
					auto WarningMessage = FText::FormatOrdered(
						INVTEXT("Actor blueprint {0} has a MOVABLE DefaultSceneRoot and child blueprints. These may "
								"break attachment of child blueprints easily, because they are not inheritable. "
								"Replace with any named component"),
						FText::FromName(BlueprintToCheck->GetFName()));

#if UE_VERSION_OLDER_THAN(5, 4, 0)
					AssetWarning(InAsset, WarningMessage);
#else
					Context.AddWarning(WarningMessage);
#endif
					break;
				}
				else if (Status == EBlueprintHasDefaultRoot::YesNonMovable)
				{
					Result = EDataValidationResult::Invalid;
					auto ErrorMessage = FText::FormatOrdered(
						INVTEXT("Actor blueprint {0} has a NON-MOVABLE DefaultSceneRoot and child blueprints. "
								"This will inevitably break attachment of child blueprints, because they are not "
								"inheritable. Replace with any named component"),
						FText::FromName(BlueprintToCheck->GetFName()));

#if UE_VERSION_OLDER_THAN(5, 4, 0)
					AssetFails(InAsset, ErrorMessage, ValidationErrors);
#else
					Context.AddError(ErrorMessage);
#endif
					break;
				}
			}

			ClassToCheck = BlueprintToCheck->ParentClass;
		}
	}

#if UE_VERSION_OLDER_THAN(5, 4, 0)
	if (Result == EDataValidationResult::Valid)
	{
		AssetPasses(InAsset);
	}
#endif
	return Result;
}
