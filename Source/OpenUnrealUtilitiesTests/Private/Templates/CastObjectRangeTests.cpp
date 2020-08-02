// Aesir Interactive GmbH, (c) 2020

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Engine/EngineTypes.h"
#include "Templates/ReverseIterator.h"
#include "Algo/IsSorted.h"

#include "Engine/EngineTypes.h"
#include "Templates/CastObjectRange.h"

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE CastObjectRange

//////////////////////////////////////////////////////////////////////////

namespace CastObjectRangeTests
{
	template<bool bSourceConst, bool bTargetConst>
	void TestObjectRange(FAutomationTestBase& AutomationTest, FString TestString)
	{
		using SourceType = typename TConditionalType<bSourceConst, const UObject*, UObject*>::Type;
		using TargetType = typename TConditionalType<bTargetConst, const AActor*, AActor*>::Type;

		// Arrange
		TArray<SourceType> ObjectArray;
		TArray<TargetType> ExpectedActorArray;
		for (int32 i = 0; i < 5; i++)
		{
			// choose nullptr for the third object
			AActor* NewActor = ((i % 3) != 0) ? NewObject<AActor>() : nullptr;
			ObjectArray.Add(NewActor);
			ExpectedActorArray.Add(NewActor);
		}

		// Act
		TArray<TargetType> ResultActorArray;
		for (TargetType Actor : CastObjectRange<AActor>(ObjectArray))
		{
			ResultActorArray.Add(Actor);
		}

		// Assert
		TestArraysEqual(AutomationTest, "CastObjectRange AActor from UObject (" + TestString + ") returned correctly cast results", ResultActorArray, ExpectedActorArray);
	}
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CastObjectRange_ConstToConst, DEFAULT_OUU_TEST_FLAGS)
{
	CastObjectRangeTests::TestObjectRange<true, true>(*this, "const to const");
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CastObjectRange_NonConstToConst, DEFAULT_OUU_TEST_FLAGS)
{
	CastObjectRangeTests::TestObjectRange<false, true>(*this, "non-const to const");
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(CastObjectRange_NonConstToNonConst, DEFAULT_OUU_TEST_FLAGS)
{
	CastObjectRangeTests::TestObjectRange<false, false>(*this, "non-const to non-const");
	return true;
}

// No 4th case, because const to non-const does not compile

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
