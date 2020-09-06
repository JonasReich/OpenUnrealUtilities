// Copyright (c) 2020 Jonas Reich

#include "OUUTests.h"
#include "Templates/StringUtils.h"

#if WITH_AUTOMATION_WORKER

#define OUU_TEST_CATEGORY OpenUnrealUtilities
#define OUU_TEST_TYPE StringUtils

//////////////////////////////////////////////////////////////////////////

enum class EStringUtilsTestEnum
{
	Alpha,
	Beta,
	Gamma
};

FString LexToString(EStringUtilsTestEnum e)
{
	switch (e)
	{
	case EStringUtilsTestEnum::Alpha: return "Alpha";
	case EStringUtilsTestEnum::Beta: return "Beta";
	case EStringUtilsTestEnum::Gamma: return "Gamma";
	default: return FString();
	}
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_Enum, DEFAULT_OUU_TEST_FLAGS)
{
	EStringUtilsTestEnum e = EStringUtilsTestEnum::Beta;
	FString S = LexToString(e);
	TestEqual("Stringified enum", S, "Beta");
	return true;
}

enum class EStringUtilsTestEnum2
{
	Alpha,
	Beta,
	Gamma
};

static_assert(TModels<CLexToStringConvertable, EStringUtilsTestEnum2>::Value == false,
	"Enum classes must not be LexToString()-convertable without a special LexToString() overload!");

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_int32_Value, DEFAULT_OUU_TEST_FLAGS)
{
	int32 I = 42;
	FString S = LexToString(I);
	TestEqual("Stringified int32 pointer", S, "42");
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_int32_Pointer_Nullptr, DEFAULT_OUU_TEST_FLAGS)
{
	int32* IPtr = nullptr;
	FString S = LexToString(IPtr);
	TestEqual("Stringified int32 pointer", S, "nullptr");
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_int32_Pointer_Valid, DEFAULT_OUU_TEST_FLAGS)
{
	int32 I = 42;
	int32* IPtr = &I;
	FString S = LexToString(IPtr);
	TestEqual("Stringified int32 pointer", S, "42");
	return true;
}

//////////////////////////////////////////////////////////////////////////

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_Object_Nullptr, DEFAULT_OUU_TEST_FLAGS)
{
	UObject* Object = nullptr;
	FString S = LexToString(Object);
	TestEqual("Stringified Object pointer", S, "Invalid");
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_Object_ValidPtr, DEFAULT_OUU_TEST_FLAGS)
{
	FName UniqueObjectName = MakeUniqueObjectName(GetTransientPackage(), UOUUTestObject::StaticClass());
	UObject* Object = NewObject<UOUUTestObject>((UObject*)GetTransientPackage(), UOUUTestObject::StaticClass(), UniqueObjectName);
	FString S = LexToString(Object);
	TestEqual("Stringified Object pointer", S, UniqueObjectName.ToString());
	return true;
}

//////////////////////////////////////////////////////////////////////////

struct FStringUtilsTestStruct
{
	FString S;
	FString ToString()
	{
		return S;
	}
};

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_ToString_Nullptr, DEFAULT_OUU_TEST_FLAGS)
{
	FStringUtilsTestStruct* StructPtr = nullptr;
	FString S = LexToString(StructPtr);
	TestEqual("Stringified Struct pointer", S, "nullptr");
	return true;
}

OUU_IMPLEMENT_SIMPLE_AUTOMATION_TEST(LexToString_ToString_ValidPtr, DEFAULT_OUU_TEST_FLAGS)
{
	FStringUtilsTestStruct Struct;
	Struct.S = "test_string";
	FStringUtilsTestStruct* StructPtr = &Struct;
	FString S = LexToString(StructPtr);
	TestEqual("Stringified Struct pointer", S, Struct.S);
	return true;
}

//////////////////////////////////////////////////////////////////////////

#undef OUU_TEST_CATEGORY
#undef OUU_TEST_TYPE

#endif
