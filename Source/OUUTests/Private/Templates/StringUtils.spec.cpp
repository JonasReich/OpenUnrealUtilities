// Copyright (c) 2021 Jonas Reich

#include "OUUTests.h"

#if WITH_AUTOMATION_WORKER

#include "Templates/StringUtils.h"

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

enum class EStringUtilsTestEnum2
{
	Alpha,
	Beta,
	Gamma
};

static_assert(TModels<CLexToStringConvertible, EStringUtilsTestEnum2>::Value == false,
	"Enum classes must not be LexToString()-convertable without a special LexToString() overload!");

struct FStringUtilsTestStruct
{
	FString S;

	FString ToString() const
	{
		return S;
	}
};

BEGIN_DEFINE_SPEC(FStringUtilsSpec, "OpenUnrealUtilities.Templates.StringUtils", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FStringUtilsSpec)

void FStringUtilsSpec::Define()
{
	Describe("LexToString", [this]()
	{
		It("should work on enums if it's overloaded", [this]()
		{
			EStringUtilsTestEnum e = EStringUtilsTestEnum::Beta;
			FString S = LexToString(e);
			TestEqual("Stringified enum", S, "Beta");
		});

		It("should work on int32 values", [this]()
		{
			int32 I = 42;
			FString S = LexToString(I);
			TestEqual("Stringified int32 value", S, "42");
		});

		Describe("executed on int32 pointers", [this]()
		{
			It("should return the string 'nullptr' if the pointer is null", [this]()
			{
				int32* IPtr = nullptr;
				FString S = LexToString(IPtr);
				TestEqual("Stringified int32 pointer", S, "nullptr");
			});

			It("should return a string conversion of the target value if the pointer is valid", [this]()
			{
				int32 I = 42;
				int32* IPtr = &I;
				FString S = LexToString(IPtr);
				TestEqual("Stringified int32 pointer", S, "42");
			});
		});

		Describe("executed on UObject pointers", [this]()
		{
			It("should return the string 'Invalid' if the pointer is null or otherwise fails an IsValid() check", [this]()
			{
				UObject* Object = nullptr;
				FString S = LexToString(Object);
				TestEqual("Stringified Object pointer", S, "Invalid");
			});

			It("should return the object name if the pointer is valid", [this]()
			{
				FName UniqueObjectName = MakeUniqueObjectName(GetTransientPackage(), UOUUTestObject::StaticClass());
				UObject* Object = NewObject<UOUUTestObject>((UObject*)GetTransientPackage(), UOUUTestObject::StaticClass(), UniqueObjectName);
				FString S = LexToString(Object);
				TestEqual("Stringified Object pointer", S, UniqueObjectName.ToString());
			});
		});

		Describe("executed on pointers/references of types that have a ToString() member function", [this]()
		{
			It("should return the string 'nullptr' if it's passed as pointer that is nullptr", [this]()
			{
				FStringUtilsTestStruct* StructPtr = nullptr;
				FString S = LexToString(StructPtr);
				TestEqual("Stringified Struct", S, "nullptr");
			});

			It("should return the result of a ToString() call on the object if it's passed as valid pointer", [this]()
			{
				FStringUtilsTestStruct Struct;
				Struct.S = "test_string";
				FStringUtilsTestStruct* StructPtr = &Struct;
				FString S = LexToString(StructPtr);
				TestEqual("Stringified Struct", S, Struct.S);
			});

			It("should return the result of a ToString() call on the object if it's passed as const reference", [this]()
			{
				FStringUtilsTestStruct Struct;
				Struct.S = "test_string";
				FString S = LexToString(Struct);
				TestEqual("Stringified Struct", S, Struct.S);
			});
		});
	});

	Describe("ArrayToString", [this]()
	{
		It("should return an empty set of brackets for an empty array", [this]()
		{
			const TArray<int32> SourceArray;
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[]");
		});

		It("should return a comma separated list of elements (, )", [this]()
		{
			const TArray<int32> SourceArray = {1, 2, 3, 4};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[1, 2, 3, 4]");
		});

		It("should quote strings", [this]()
		{
			const TArray<FString> SourceArray = {"apple", "banana", "citrus", "dragon fruit"};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should quote c-strings", [this]()
		{
			const TArray<FString> SourceStringArray = {"apple", "banana", "citrus", "dragon fruit"};
			const TArray<const TCHAR*> SourceCharPtrArray = {*SourceStringArray[0], *SourceStringArray[1], *SourceStringArray[2], *SourceStringArray[3]};
			const FString Result = ArrayToString(SourceCharPtrArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should quote names", [this]()
		{
			const TArray<FName> SourceArray = {"apple", "banana", "citrus", "dragon fruit"};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should quote texts", [this]()
		{
			const TArray<FText> SourceArray = {INVTEXT("apple"), INVTEXT("banana"), INVTEXT("citrus"), INVTEXT("dragon fruit")};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should work with custom types like enums", [this]()
		{
			const TArray<EStringUtilsTestEnum> SourceArray = {
				EStringUtilsTestEnum::Alpha,
				EStringUtilsTestEnum::Beta,
				EStringUtilsTestEnum::Gamma
			};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[Alpha, Beta, Gamma]");
		});
	});
}

#endif
