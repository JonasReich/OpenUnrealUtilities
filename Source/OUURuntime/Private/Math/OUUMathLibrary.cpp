// Copyright (c) 2023 Jonas Reich & Contributors

#include "Math/OUUMathLibrary.h"

float UOUUMathLibrary::AngleBetweenVectors(FVector A, FVector B)
{
	A.Normalize();
	B.Normalize();
	return FMath::RadiansToDegrees(FMath::Acos(FVector::DotProduct(A, B)));
}

float UOUUMathLibrary::SignedAngleBetweenVectors(const FVector& A, const FVector& B, const FVector& Up)
{
	const float Angle = AngleBetweenVectors(A, B);
	return (FVector::DotProduct(Up, FVector::CrossProduct(A, B)) < 0) ? Angle : -Angle;
}
