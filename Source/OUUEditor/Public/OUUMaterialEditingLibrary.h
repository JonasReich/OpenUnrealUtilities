// Copyright (c) 2022 Jonas Reich

#pragma once

#include "CoreMinimal.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUMaterialEditingLibrary.generated.h"

class UMaterialFunction;
class UMaterial;
class UMaterialExpressionMakeMaterialAttributes;

struct FExpressionInput;

/** Library to make some more advanced material edits than the base set provided in UMaterialEditingLibrary */
UCLASS()
class UOUUMaterialEditingLibrary : public UBlueprintFunctionLibrary
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

private:
	static void CopyInputConnection(FExpressionInput* From, FExpressionInput* To);
	static void CopyMaterialAttributeConnections(
		UMaterial* SourceMaterial,
		UMaterialExpressionMakeMaterialAttributes* TargetMakeMaterialAttributes);
};
