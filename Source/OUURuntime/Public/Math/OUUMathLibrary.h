// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUMathLibrary.generated.h"

/**
 * Library for various math utilities not included in FMath or U
 */
UCLASS()
class OUURUNTIME_API UOUUMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Get the absolute angle between two vectors A and B. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Math")
	static float AngleBetweenVectors(FVector A, FVector B);

	/** Get the signed angle between two vectors A and B, determining the sign with an Up vector. */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Math")
	static float SignedAngleBetweenVectors(const FVector& A, const FVector& B, const FVector& Up);

	/** Clamp a value into a range, using the fallback values if the range is not bound at the respective end. */
	template <typename T>
	static T ClampToRange(T Value, TRange<T>& Range, T FalbackMin, T FallbackMax)
	{
		return FMath::Clamp(
			Value,
			Range.HasLowerBound() ? Range.GetLowerBoundValue() : FalbackMin,
			Range.HasUpperBound() ? Range.GetUpperBoundValue() : FallbackMax);
	}

	/**	Clamp a value into a range, using numeric type limits if the range is not bound. */
	template <typename T>
	static T ClampToRange(T Value, TRange<T>& Range)
	{
		return FMath::Clamp(
			Value,
			Range.HasLowerBound() ? Range.GetLowerBoundValue() : TNumericLimits<T>::Min(),
			Range.HasUpperBound() ? Range.GetUpperBoundValue() : TNumericLimits<T>::Max());
	}

	static float LinearValueToNormalizedLogScale(float Value, float ValueRangeMin, float ValueRangeMax)
	{
		const float Offset = FMath::Loge(ValueRangeMin);
		return (FMath::Loge(Value) - Offset) / (FMath::Loge(ValueRangeMax) - Offset);
	}
};
