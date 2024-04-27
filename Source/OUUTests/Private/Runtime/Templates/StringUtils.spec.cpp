// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "Templates/StringUtils.h"
	#include "UObject/Package.h"

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

static_assert(
	TModels<CLexToStringConvertible, EStringUtilsTestEnum>::Value == true,
	"Enum classes must be LexToString()-convertible with a special LexToString() overload!");

enum class EStringUtilsTestEnum2
{
	Alpha,
	Beta,
	Gamma
};

static_assert(
	TModels<CLexToStringConvertible, EStringUtilsTestEnum2>::Value == false,
	"Enum classes must not be LexToString()-convertible without a special LexToString() overload!");

struct FStringUtilsTestStruct
{
	FString S;

	FString ToString() const { return S; }
};

static_assert(
	TModels<CLexToStringConvertible, FStringUtilsTestStruct>::Value,
	"Structs with ToString() member function must be lex to string convertible if 'Templates/StringUtils.h' is "
	"included");

BEGIN_DEFINE_SPEC(FStringUtilsSpec, "OpenUnrealUtilities.Runtime.Templates.StringUtils", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FStringUtilsSpec)

void FStringUtilsSpec::Define()
{
	Describe("LexToString", [this]() {
		It("should work on enums if it's overloaded", [this]() {
			constexpr EStringUtilsTestEnum e = EStringUtilsTestEnum::Beta;
			const FString S = LexToString(e);
			TestEqual("String converted enum", S, "Beta");
		});

		It("should work on int32 values", [this]() {
			constexpr int32 I = 42;
			const FString S = LexToString(I);
			TestEqual("String converted int32 value", S, "42");
		});

		Describe("executed on int32 pointers", [this]() {
			It("should return the string 'nullptr' if the pointer is null", [this]() {
				const int32* IPtr = nullptr;
				const FString S = LexToString(IPtr);
				TestEqual("String converted int32 pointer", S, "nullptr");
			});

			It("should return a string conversion of the target value if the pointer is valid", [this]() {
				constexpr int32 I = 42;
				const int32* IPtr = &I;
				const FString S = LexToString(IPtr);
				TestEqual("String converted int32 pointer", S, "42");
			});
		});

		Describe("executed on UObject pointers", [this]() {
			It("should return the string 'None' if the pointer is null or otherwise fails an IsValid() check",
			   [this]() {
				   const UObject* Object = nullptr;
				   const FString S = LexToString(Object);
				   TestEqual("String converted Object pointer", S, "None");
			   });

			It("should return the object name if the pointer is valid", [this]() {
				const FName UniqueObjectName =
					MakeUniqueObjectName(GetTransientPackage(), UOUUTestObject::StaticClass());
				const UObject* Object = NewObject<UOUUTestObject>(
					(UObject*)GetTransientPackage(),
					UOUUTestObject::StaticClass(),
					UniqueObjectName);
				const FString S = LexToString(Object);
				TestEqual("String converted Object pointer", S, UniqueObjectName.ToString());
			});
		});

		Describe("executed on pointers/references of types that have a ToString() member function", [this]() {
			It("should return the string 'nullptr' if it's passed as pointer that is nullptr", [this]() {
				const FStringUtilsTestStruct* StructPtr = nullptr;
				const FString S = LexToString(StructPtr);
				TestEqual("String converted Struct", S, "nullptr");
			});

			It("should return the result of a ToString() call on the object if it's passed as valid pointer", [this]() {
				FStringUtilsTestStruct Struct;
				Struct.S = "test_string";
				const FStringUtilsTestStruct* StructPtr = &Struct;
				const FString S = LexToString(StructPtr);
				TestEqual("String converted Struct", S, Struct.S);
			});

			It("should return the result of a ToString() call on the object if it's passed as const reference",
			   [this]() {
				   FStringUtilsTestStruct Struct;
				   Struct.S = "test_string";
				   const FString S = LexToString(Struct);
				   TestEqual("String converted Struct", S, Struct.S);
			   });
		});
	});

	Describe("ArrayToString", [this]() {
		It("should return an empty set of brackets for an empty array", [this]() {
			const TArray<int32> SourceArray;
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[]");
		});

		It("should return a comma separated list of elements (, )", [this]() {
			const TArray<int32> SourceArray = {1, 2, 3, 4};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[1, 2, 3, 4]");
		});

		It("should quote strings", [this]() {
			const TArray<FString> SourceArray = {"apple", "banana", "citrus", "dragon fruit"};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should quote c-strings", [this]() {
			const TArray<FString> SourceStringArray = {"apple", "banana", "citrus", "dragon fruit"};
			const TArray<const TCHAR*> SourceCharPtrArray =
				{*SourceStringArray[0], *SourceStringArray[1], *SourceStringArray[2], *SourceStringArray[3]};
			const FString Result = ArrayToString(SourceCharPtrArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should quote names", [this]() {
			const TArray<FName> SourceArray = {"apple", "banana", "citrus", "dragon fruit"};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should quote texts", [this]() {
			const TArray<FText> SourceArray =
				{INVTEXT("apple"), INVTEXT("banana"), INVTEXT("citrus"), INVTEXT("dragon fruit")};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[\"apple\", \"banana\", \"citrus\", \"dragon fruit\"]");
		});

		It("should work with custom types like enums", [this]() {
			const TArray<EStringUtilsTestEnum> SourceArray =
				{EStringUtilsTestEnum::Alpha, EStringUtilsTestEnum::Beta, EStringUtilsTestEnum::Gamma};
			const FString Result = ArrayToString(SourceArray);
			SPEC_TEST_EQUAL(Result, "[Alpha, Beta, Gamma]");
		});
	});
}

#endif
