// Copyright (c) 2024 Jonas Reich & Contributors

#include "AssetValidation/OUUBlueprintValidator.h"

#include "Engine/SCS_Node.h"

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

bool UOUUBlueprintValidator::CanValidateAsset_Implementation(UObject* InAsset) const
{
	// For now: do not run this validator in cook.
	if (IsValid(InAsset) == false)
	{
		return false;
	}

	return InAsset->IsA<UBlueprint>();
}

EDataValidationResult UOUUBlueprintValidator::ValidateLoadedAsset_Implementation(
	UObject* InAsset,
	TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	UBlueprint* Blueprint = Cast<UBlueprint>(InAsset);

	if (IsValid(Blueprint) == false)
	{
		AssetFails(InAsset, INVTEXT("Asset is not a blueprint"), IN OUT ValidationErrors);
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
					AssetWarning(
						InAsset,
						FText::FormatOrdered(
							INVTEXT(
								"Actor blueprint {0} has a MOVABLE DefaultSceneRoot and child blueprints. These may "
								"break attachment of child blueprints easily, because they are not inheritable. "
								"Replace with any named component"),
							FText::FromName(BlueprintToCheck->GetFName())));
					break;
				}
				else if (Status == EBlueprintHasDefaultRoot::YesNonMovable)
				{
					Result = EDataValidationResult::Invalid;
					AssetFails(
						InAsset,
						FText::FormatOrdered(
							INVTEXT("Actor blueprint {0} has a NON-MOVABLE DefaultSceneRoot and child blueprints. "
									"This will inevitably break attachment of child blueprints, because they are not "
									"inheritable. Replace with any named component"),
							FText::FromName(BlueprintToCheck->GetFName())),
						ValidationErrors);
					break;
				}
			}

			ClassToCheck = BlueprintToCheck->ParentClass;
		}
	}

	if (Result == EDataValidationResult::Valid)
	{
		AssetPasses(InAsset);
	}
	return Result;
}
