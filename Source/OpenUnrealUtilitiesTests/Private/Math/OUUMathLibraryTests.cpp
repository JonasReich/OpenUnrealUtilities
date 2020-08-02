// Copyright (c) 2020 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Math/OUUMathLibrary.h"
#include "Engine/EngineTypes.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE MathLibrary

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(AngleBetweenVectors, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0|+180")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+0,Y=+1,Z=+0|X=+0,Y=-1,Z=+0|+180")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+0,Y=-1,Z=+0|X=+0,Y=+1,Z=+0|+180")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=+0,Y=+1,Z=+0|+90")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=+0,Y=+0,Z=+1|+90")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=+1,Y=+1,Z=+0|+45")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(AngleBetweenVectors)
{
	// Arrange
	TArray<FString> Args;
	Parameters.ParseIntoArray(Args, TEXT("|"));
	FVector A;
	A.InitFromString(Args[0]);	
	FVector B;
	B.InitFromString(Args[1]);
	float ExpectedAngle;
	LexTryParseString(ExpectedAngle, *Args[2]);
	
	// Act
	float ActualAngle = UOUUMathLibrary::AngleBetweenVectors(A, B);

	// Assert
	FString DisplayString = FString::Printf(TEXT("Angle between vectors %s and %s"), *Args[0], *Args[1]);
	TestEqual(DisplayString, ActualAngle, ExpectedAngle);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(SignedAngleBetweenVectors, DEFAULT_OUU_TEST_FLAGS)
// 100% opposing vectors will always result in -180 because cross product is (0,0,0)
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0|X=+0,Y=+1,Z=+0|-180")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0|X=+0,Y=-1,Z=+0|-180")
// ...so we add a variant with a little difference on one axis that does not affect the angle
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0.0001|X=+0,Y=+1,Z=+0|+180")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0.0001|X=+0,Y=-1,Z=+0|-180")
// for smaller angles everything is fine
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=+0,Y=+0,Z=+1|X=+0,Y=+1,Z=+0|+90")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=+0,Y=+0,Z=+1|X=+0,Y=-1,Z=+0|-90")
// changing vectors A and B (compared to the two cases directly above) should result in opposite signs
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+0,Y=+0,Z=+1|X=+1,Y=+0,Z=+0|X=+0,Y=+1,Z=+0|-90")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+0,Y=+0,Z=+1|X=+1,Y=+0,Z=+0|X=+0,Y=-1,Z=+0|+90")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(SignedAngleBetweenVectors)
{
	// Arrange
	TArray<FString> Args;
	Parameters.ParseIntoArray(Args, TEXT("|"));
	FVector A;
	A.InitFromString(Args[0]);
	FVector B;
	B.InitFromString(Args[1]);
	FVector Up;
	Up.InitFromString(Args[2]);
	float ExpectedAngle;
	LexTryParseString(ExpectedAngle, *Args[3]);

	// Act
	float ActualAngle = UOUUMathLibrary::SignedAngleBetweenVectors(A, B, Up);

	// Assert
	FString DisplayString = FString::Printf(TEXT("Signed angle between vectors %s and %s"), *Args[0], *Args[1]);
	TestEqual(DisplayString, ActualAngle, ExpectedAngle);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(ClampToRange, DEFAULT_OUU_TEST_FLAGS)
OUU_COMPLEX_AUTOMATION_TESTCASE("0|1|3|1")
OUU_COMPLEX_AUTOMATION_TESTCASE("2|1|3|2")
OUU_COMPLEX_AUTOMATION_TESTCASE("4|1|3|3")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(ClampToRange)
{
	// Arrange
	TArray<FString> Args;
	Parameters.ParseIntoArray(Args, TEXT("|"));
	
	float InValue = 0;
	LexTryParseString(InValue, *Args[0]);
	float Min = 0;
	float Max = 1;
	LexTryParseString(Min, *Args[1]);
	LexTryParseString(Max, *Args[2]);
	TRange<float> Range{ Min, Max };
	float ExpectedResult = 0;
	LexTryParseString(ExpectedResult, *Args[3]);

	// Act
	float ActualResult = UOUUMathLibrary::ClampToRange(InValue, Range);

	// Assert
	FString DisplayString = FString::Printf(TEXT("Signed angle between vectors %s and %s"), *Args[0], *Args[1]);
	TestEqual(DisplayString, ActualResult, ExpectedResult);

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
