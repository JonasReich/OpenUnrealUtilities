// Copyright (c) 2021 Jonas Reich

#include "Math/OUUMathLibrary.h"

float UOUUMathLibrary::AngleBetweenVectors(FVector A, FVector B)
{
	A.Normalize();
	B.Normalize();
	return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(A, B)));
}

float UOUUMathLibrary::SignedAngleBetweenVectors(FVector A, FVector B, FVector Up)
{
	const float Angle = AngleBetweenVectors(A, B);
	return (FVector::DotProduct(Up, FVector::CrossProduct(A, B)) < 0) ? Angle : -Angle;
}
