// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/CastObjectRange.h"
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

namespace CastObjectRangeTests
{
	template<bool bSourceConst, bool bTargetConst, bool bCopySourceArray, bool bForwardConstArray,
		typename SourceType = typename TConditionalType<bSourceConst, const UObject*, UObject*>::Type,
		typename TargetType = typename TConditionalType<bTargetConst, const AActor*, AActor*>::Type,
		typename ForwardArrayType = typename TConditionalType<bForwardConstArray, const TArray<SourceType>&, TArray<SourceType>&>::Type>
	void TestObjectRange(FAutomationTestBase& AutomationTest, FString TestString)
	{
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
		if (bCopySourceArray)
		{
			for (TargetType Actor : CastObjectRange<AActor>(CopyTemp(ObjectArray)))
			{
				ResultActorArray.Add(Actor);
			}
		}
		else
		{
			for (TargetType Actor : CastObjectRange<AActor>(Forward<ForwardArrayType>(ObjectArray)))
			{
				ResultActorArray.Add(Actor);
			}
		}

		// Assert
		TestArraysEqual(AutomationTest, "CastObjectRange (" + TestString + ")", ResultActorArray, ExpectedActorArray);
	}
}

//////////////////////////////////////////////////////////////////////////

BEGIN_DEFINE_SPEC(FCastObjectRangeSpec, "OpenUnrealUtilities.Templates.CastObjectRange", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FCastObjectRangeSpec)
void FCastObjectRangeSpec::Define()
{
	Describe("when the collection is passed by non-const reference", [this]()
	{
		It("should cast elements of TArray<const UObject*>& to const AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<true, true, false, false>(*this, "const to const by ref");
		});

		It("should cast elements of TArray<UObject*>& to const AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<false, true, false, false>(*this, "non-const to const by ref");
		});

		It("should cast elements of TArray<UObject*>& to AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<false, false, false, false>(*this, "non-const to non-const by ref");
		});

		// No 4th case: cannot convert from 'const AActor *' to 'TargetType' with [ TargetType=AActor * ]
	});
	
	Describe("when the collection is passed by const reference", [this]()
	{
		It("should cast elements of const TArray<const UObject*>& to const AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<true, true, false, true>(*this, "const to const by ref");
		});

		It("should cast elements of const TArray<UObject*>& to const AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<false, true, false, true>(*this, "non-const to const by ref");
		});

		// No 3rd case: cannot convert from 'const AActor *' to 'TargetType' with [ TargetType=AActor * ]
		// No 4th case: cannot convert from 'const AActor *' to 'TargetType' with [ TargetType=AActor * ]
	});

	Describe("when the collection is passed by value", [this]()
	{
		It("should cast elements of TArray<const UObject*> to const AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<true, true, true, false>(*this, "const to const by value");
		});

		It("should cast elements of TArray<UObject*> to const AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<false, true, true, false>(*this, "non-const to const by value");
		});

		It("should cast elements of TArray<UObject*> to AActor*", [this]()
		{
			CastObjectRangeTests::TestObjectRange<false, false, true, false>(*this, "non-const to non-const by value");
		});

		// No 4th case: cannot convert from 'const AActor *' to 'TargetType' with [ TargetType=AActor * ]
	});
}

#endif
