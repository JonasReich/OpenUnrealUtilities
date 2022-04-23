// Copyright (c) 2022 Jonas Reich

#pragma once

#include "Materials/MaterialLayersFunctions.h"

#include "OUUMaterialParametersOverivew_EditorObject.generated.h"

class UMaterial;
class UMaterialExpression;

USTRUCT()
struct FSortedMaterialParameter
{
	GENERATED_BODY()
public:
	enum class ESource
	{
		Undefined,
		Material,
		MaterialFunction
	};

	FSortedMaterialParameter() = default;
	FSortedMaterialParameter(FMaterialParameterInfo InInfo, int32 InSortPriority) :
		Info(InInfo), SortPriority(InSortPriority)
	{
	}

	FMaterialParameterInfo Info;
	ESource Source = ESource::Undefined;
	int32 SortPriority = 0;

	UPROPERTY()
	UMaterialExpression* Expression = nullptr;
};

FORCEINLINE bool operator<(const FSortedMaterialParameter& A, const FSortedMaterialParameter& B)
{
	/*
	if (A.SortPriority != B.SortPriority)
		return A.SortPriority < B.SortPriority;
	*/
	return A.Info.Name.ToString() < B.Info.Name.ToString();
}

FORCEINLINE bool operator<(const TSharedPtr<FSortedMaterialParameter>& A, const TSharedPtr<FSortedMaterialParameter>& B)
{
	return (A.IsValid() && B.IsValid()) ? (*A.Get() < *B.Get()) : A.IsValid();
}

UCLASS()
class UOUUMaterialParametersOverivew_EditorObject : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Transient)
	UMaterial* TargetMaterial;

	TArray<TSharedPtr<FSortedMaterialParameter>> Parameters;
};
