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
	float Angle = AngleBetweenVectors(A, B);
	UE_LOG(LogTemp, Log, TEXT("A:%s, B:%s, Up:%s, Cross:%s"),
		*A.ToString(),
		*B.ToString(),
		*Up.ToString(),
		*FVector::CrossProduct(A, B).ToString());
	return (FVector::DotProduct(Up, FVector::CrossProduct(A, B)) < 0) ? Angle : -Angle;
}
