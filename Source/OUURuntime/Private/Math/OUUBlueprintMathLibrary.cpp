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
