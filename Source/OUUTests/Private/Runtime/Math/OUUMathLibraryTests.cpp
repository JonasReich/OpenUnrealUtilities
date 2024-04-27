// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Engine/EngineTypes.h"
	#include "Math/OUUMathLibrary.h"

	#define OUU_TEST_CATEGORY OpenUnrealUtilities.Runtime.Math
	#define OUU_TEST_TYPE	  MathLibrary

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
	const FAutomationTestParameterParser Parser{Parameters};
	const FVector A = Parser.GetValue<FVector>(0);
	const FVector B = Parser.GetValue<FVector>(1);
	const float ExpectedAngle = Parser.GetValue<float>(2);

	// Act
	const float ActualAngle = UOUUMathLibrary::AngleBetweenVectors(A, B);

	// Assert
	const FString DisplayString = FString::Printf(TEXT("Angle between vectors %s and %s"), *A.ToString(), *B.ToString());
	TestEqual(DisplayString, ActualAngle, ExpectedAngle);

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_BEGIN(SignedAngleBetweenVectors, DEFAULT_OUU_TEST_FLAGS)
// 100% opposing vectors will always result in -180 because cross product is (0,0,0)
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0|X=+0,Y=+1,Z=+0|-180")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0|X=+0,Y=-1,Z=+0|-180")

// ...so we add a variant with a little difference on one axis that does not affect the angle
// OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0.0001|X=+0,Y=+1,Z=+0|+180")
// OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=-1,Y=+0,Z=+0.0001|X=+0,Y=-1,Z=+0|-180")
// !!! -> this used to be the case before Epic introduced high precision floats/doubles for FVector

// for smaller angles everything is fine
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=+0,Y=+0,Z=+1|X=+0,Y=+1,Z=+0|+90")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+1,Y=+0,Z=+0|X=+0,Y=+0,Z=+1|X=+0,Y=-1,Z=+0|-90")

// changing vectors A and B (compared to the two cases directly above) should result in opposite signs
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+0,Y=+0,Z=+1|X=+1,Y=+0,Z=+0|X=+0,Y=+1,Z=+0|-90")
OUU_COMPLEX_AUTOMATION_TESTCASE("X=+0,Y=+0,Z=+1|X=+1,Y=+0,Z=+0|X=+0,Y=-1,Z=+0|+90")
OUU_IMPLEMENT_COMPLEX_AUTOMATION_TEST_END(SignedAngleBetweenVectors)
{
	// Arrange
	const FAutomationTestParameterParser Parser{Parameters};
	const FVector A = Parser.GetValue<FVector>(0);
	const FVector B = Parser.GetValue<FVector>(1);
	const FVector Up = Parser.GetValue<FVector>(2);
	const float ExpectedAngle = Parser.GetValue<float>(3);

	// Act
	const float ActualAngle = UOUUMathLibrary::SignedAngleBetweenVectors(A, B, Up);

	// Assert
	const FString DisplayString = FString::Printf(
		TEXT("Signed angle between vectors %s and %s with up: %s"),
		*A.ToString(),
		*B.ToString(),
		*Up.ToString());
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
	const FAutomationTestParameterParser Parser{Parameters};
	const float InValue = Parser.GetValue<float>(0);
	const float Min = Parser.GetValue<float>(1);
	const float Max = Parser.GetValue<float>(2);
	TRange<float> Range{Min, Max};
	const float ExpectedResult = Parser.GetValue<float>(3);

	// Act
	const float ActualResult = UOUUMathLibrary::ClampToRange(InValue, Range);

	// Assert
	const FString DisplayString = FString::Printf(TEXT("%f clamped between %f and %f"), InValue, Min, Max);
	TestEqual(DisplayString, ActualResult, ExpectedResult);

	return true;
}

//////////////////////////////////////////////////////////////////////////

	#undef OUU_TEST_CATEGORY
	#undef OUU_TEST_TYPE

#endif
