// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUMaterialEditingLibrary.generated.h"

class UMaterial;
class UMaterialFunction;
class UMaterialExpression;
class UMaterialExpressionMakeMaterialAttributes;

struct FExpressionInput;

/** Library to make some more advanced material edits than the base set provided in UMaterialEditingLibrary */
UCLASS()
class OUUEDITOR_API UOUUMaterialEditingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Editor|Material Editing")
	static void ConvertMaterialToMaterialAttributes(UMaterial* Material);

	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Editor|Material Editing")
	static void InsertMaterialFunctionBeforeResult(
		UMaterial* Material,
		UMaterialFunction* MaterialFunction,
		bool bOnlyAddIfNotPresent = true);

	/**
	 * Open an editor for the material containing this expression and jump to the node.
	 * Jumping to the node only works for parameter expressions.
	 */
	UFUNCTION(BlueprintCallable, Category = "Open Unreal Utilities|Editor|Material Editing")
	static void OpenMaterialEditorAndJumpToExpression(UMaterialExpression* MaterialExpression);

private:
	static void CopyInputConnection(const FExpressionInput* From, FExpressionInput* To);
	static void CopyMaterialAttributeConnections(
		UMaterial* SourceMaterial,
		UMaterialExpressionMakeMaterialAttributes* TargetMakeMaterialAttributes);
};
