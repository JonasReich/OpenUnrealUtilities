// Copyright (c) 2021 Jonas Reich

#include "Math/OUUBlueprintMathLibrary.h"

FRotator UOUUBlueprintMathLibrary::Add_RotatorRotator(const FRotator& A, const FRotator& B)
{
	return A + B;
}

FRotator UOUUBlueprintMathLibrary::Subtract_RotatorRotator(const FRotator& A, const FRotator& B)
{
	return A - B;
}

FVector UOUUBlueprintMathLibrary::GetTransformForwardVector(const FTransform& Transform)
{
	return Transform.GetUnitAxis(EAxis::X);
}

FVector UOUUBlueprintMathLibrary::GetTransformRightVector(const FTransform& Transform)
{
	return Transform.GetUnitAxis(EAxis::Y);
}

FVector UOUUBlueprintMathLibrary::GetTransformUpVector(const FTransform& Transform)
{
	return Transform.GetUnitAxis(EAxis::Z);
}
