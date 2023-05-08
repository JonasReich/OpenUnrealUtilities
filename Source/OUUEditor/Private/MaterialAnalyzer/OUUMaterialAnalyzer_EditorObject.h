// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "MaterialAnalyzer/OUUMaterialAnalyzer_ParameterData.h"

#include "OUUMaterialAnalyzer_EditorObject.generated.h"

class UMaterial;

UCLASS()
class UOUUMaterialAnalyzer_EditorObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	UMaterial* TargetMaterial;

	TArray<TSharedPtr<FOUUMaterialAnalyzer_ParameterData>> Parameters;
};
