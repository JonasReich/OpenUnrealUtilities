// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"

#include "OUUBlueprintMathLibrary.generated.h"

/**
 * Utility class to expose math struct operators and FMath library functions to blueprint
 * that are not exposed by default in UKismetMathLibrary.
 * Custom functionality that is not yet path of any engine utility class is defined in UOUUMathLibrary instead.
 */
UCLASS()
class OUUBLUEPRINTRUNTIME_API UOUUBlueprintMathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	//
	// Rotator functions.
	//

	/** @returns addition of Rotator A and Rotator B (A + B) */
	UFUNCTION(
		BlueprintPure,
		meta =
			(DisplayName = "rotator + rotator",
			 CompactNodeTitle = "+",
			 Keywords = "+ add plus",
			 CommutativeAssociativeBinaryOperator = "true"),
		Category = "Open Unreal Utilities|Math|Rotator")
	static FRotator Add_RotatorRotator(const FRotator& A, const FRotator& B);

	/** @returns subtraction of Rotator A and Rotator B (A - B) */
	UFUNCTION(
		BlueprintPure,
		meta = (DisplayName = "rotator - rotator", CompactNodeTitle = "-", Keywords = "- subtract minus"),
		Category = "Open Unreal Utilities|Math|Rotator")
	static FRotator Subtract_RotatorRotator(const FRotator& A, const FRotator& B);

	/** @returns the forward vector of the given transform */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Math|Transform")
	static FVector GetTransformForwardVector(const FTransform& Transform);

	/** @returns the right vector of the given transform */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Math|Transform")
	static FVector GetTransformRightVector(const FTransform& Transform);

	/** @returns the up vector of the given transform */
	UFUNCTION(BlueprintPure, Category = "Open Unreal Utilities|Math|Transform")
	static FVector GetTransformUpVector(const FTransform& Transform);
};
