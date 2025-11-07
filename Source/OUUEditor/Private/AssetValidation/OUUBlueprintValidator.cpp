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

bool UOUUBlueprintValidator::CanValidateAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& InContext) const
{
	if (IsValid(InAsset) == false)
	{
		return false;
	}

	return InAsset->IsA<UBlueprint>();
}

EDataValidationResult UOUUBlueprintValidator::ValidateLoadedAsset_Implementation(
	const FAssetData& InAssetData,
	UObject* InAsset,
	FDataValidationContext& Context)
{
	const UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);

	if (IsValid(Blueprint) == false)
	{
		Context.AddError(INVTEXT("Asset is not a blueprint"));
		return EDataValidationResult::Invalid;
	}

	if (Blueprint->ParentClass->IsChildOf<AActor>())
	{
		const UClass* ClassToCheck = Blueprint->GeneratedClass;

		while (ClassToCheck->IsInBlueprint())
		{
			TArray<UClass*> ChildClasses;
			GetDerivedClasses(ClassToCheck, ChildClasses);

			const auto* BlueprintToCheck = CastChecked<UBlueprint>(ClassToCheck->ClassGeneratedBy);
			if (ChildClasses.Num() > 0)
			{
				const auto Status = BlueprintHasNonMovableDefaultRoot(*BlueprintToCheck);

				if (Status == EBlueprintHasDefaultRoot::YesMovable)
				{
					const auto WarningMessage = FText::FormatOrdered(
						INVTEXT("Actor blueprint {0} has a MOVABLE DefaultSceneRoot and child blueprints. These may "
								"break attachment of child blueprints easily, because they are not inheritable. "
								"Consider replacing it with any named component - keep in mind that replacing the root"
								"component of already placed actors can break attachment of manually added components!"),
						FText::FromName(BlueprintToCheck->GetFName()));

					Context.AddWarning(WarningMessage);
					break;
				}
				else if (Status == EBlueprintHasDefaultRoot::YesNonMovable)
				{
					const auto ErrorMessage = FText::FormatOrdered(
						INVTEXT("Actor blueprint {0} has a NON-MOVABLE DefaultSceneRoot and child blueprints. "
								"This will inevitably break attachment of child blueprints, because they are not "
								"inheritable. Replace with any named component"),
						FText::FromName(BlueprintToCheck->GetFName()));

					Context.AddError(ErrorMessage);
					return EDataValidationResult::Invalid;
				}
			}

			ClassToCheck = BlueprintToCheck->ParentClass;
		}
	}
	return EDataValidationResult::Valid;
}
