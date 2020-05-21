// Copyright (c) 2020 Jonas Reich

#include "OUUMathLibrary.h"

float UOUUMathLibrary::AngleBetweenVectors(FVector A, FVector B)
{
	A.Normalize();
	B.Normalize();
	return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(A, B)));
}

float UOUUMathLibrary::SignedAngleBetweenVectors(FVector A, FVector B, FVector Up)
{
	float Angle = AngleBetweenVectors(A, B);
	return (FVector::DotProduct(Up, FVector::CrossProduct(A, B)) < 0) ? Angle : -Angle;
}
