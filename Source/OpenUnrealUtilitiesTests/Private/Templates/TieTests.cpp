// Copyright (c) 2020 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/Tie.h"
#include "Engine/EngineTypes.h"

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE Tie

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(UnpackMixedTuple, DEFAULT_OUU_TEST_FLAGS)
{
	using ResultTuple = TTuple < int32, FString >;
	// Arrange
	int32 i = 0;
	FString str = "before";

	auto foo = []() -> ResultTuple { return ResultTuple{ 42, "after" }; };

	// Act
	Tie(i, str) = foo();

	// Assert
	TestTrue("i was tied to first tuple value", i == 42);
	TestTrue("str was tied to second tuple value", str == "after");

	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(UnpackStringTuples, DEFAULT_OUU_TEST_FLAGS)
{
	using ResultTuple = TTuple < FString, FString, FString >;

	// Arrange
	int32 i = 0;
	FString str1, str2, str3, str4, str5, str6, str7, str8;

	auto foo = []() { return ResultTuple{ "result",  "as",  "expected" }; };
	ResultTuple localVar{ "local var", "also", "fine" };

	// Act
	Tie(str1, str2, str3) = foo();
	Tie(str4, str5, str6) = localVar;
	Tie(str7, str8) = TTuple<FString, FString>{ "inline", "too" };

	// Assert
	TestTrue("All values moved from function result", (str1 == "result" && str2 == "as" && str3 == "expected"));
	TestTrue("All values copied from local variable", (str4 == "local var" && str5 == "also" && str6 == "fine"));
	TestTrue("All values moved from inline variable", (str7 == "inline" && str8 == "too"));

	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
