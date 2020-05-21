// Copyright (c) 2020 Jonas Reich

#include "Math/OUUBlueprintMathLibrary.h"

FRotator UOUUBlueprintMathLibrary::Add_RotatorRotator(const FRotator& A, const FRotator& B)
{
	return A + B;
}

FRotator UOUUBlueprintMathLibrary::Subtract_RotatorRotator(const FRotator& A, const FRotator& B)
{
	return A - B;
}
