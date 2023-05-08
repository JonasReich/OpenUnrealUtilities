// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "CoreMinimal.h"

#include "MaterialAnalyzer/OUUMaterialAnalyzer.h"
#include "Materials/MaterialLayersFunctions.h"

#include "OUUMaterialAnalyzer_ParameterData.generated.h"

class UMaterialExpression;

USTRUCT()
struct FOUUMaterialAnalyzer_ParameterData
{
	GENERATED_BODY()
public:
	FOUUMaterialAnalyzer_ParameterData() = default;
	FOUUMaterialAnalyzer_ParameterData(FMaterialParameterInfo InInfo, int32 InSortPriority) :
		Info(InInfo), SortPriority(InSortPriority)
	{
	}

	using ESource = OUU::Editor::Private::MaterialAnalyzer::ESource;

	FMaterialParameterInfo Info;
	ESource Source = ESource::Undefined;
	int32 SortPriority = 0;

	UPROPERTY()
	UMaterialExpression* Expression = nullptr;
};

FORCEINLINE bool operator<(const FOUUMaterialAnalyzer_ParameterData& A, const FOUUMaterialAnalyzer_ParameterData& B)
{
	// Ignore SortPriority for now. We don't have any group data.
	/*
	if (A.SortPriority != B.SortPriority)
		return A.SortPriority < B.SortPriority;
	*/
	return A.Info.Name.ToString() < B.Info.Name.ToString();
}

FORCEINLINE bool operator<(
	const TSharedPtr<FOUUMaterialAnalyzer_ParameterData>& A,
	const TSharedPtr<FOUUMaterialAnalyzer_ParameterData>& B)
{
	return (A.IsValid() && B.IsValid()) ? (*A.Get() < *B.Get()) : A.IsValid();
}
