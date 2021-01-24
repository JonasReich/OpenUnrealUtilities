// Copyright (c) 2021 Jonas Reich

#pragma once

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_WORKER

#include "Traits/StringConversionTraits.h"
#include "Templates/IsEnumClass.h"
#include "Templates/UnrealTypeTraits.h"
#include "OUUTestsModule.h"

namespace OUUTests_Internal
{
	template<typename T, typename EnableType = typename TEnableIf<TModels<CLexTryParseString_Parseable, T>::Value == true>::Type>
	T ParseValue(const FString& s)
	{
		T Result;
		if (LexTryParseString(Result, *s))
			return Result;
		return T();
	}

	template<typename T, typename EnableType = typename TEnableIf<TModels<CLexTryParseString_Parseable, T>::Value == false &&
		TModels<CLexFromString_Parseable, T>::Value == true>::Type,
		typename T2 = void>
		T ParseValue(const FString& s)
	{
		T Result;
		LexFromString(Result, *s);
		return Result;
	}

	/** For enum classes: Parse from int if LexTryParseString and LexFromString are not overloaded */
	template<typename T, typename EnableType = typename TEnableIf<TIsEnumClass<T>::Value == true &&
		TModels<CLexTryParseString_Parseable, T>::Value == false && 
		TModels<CLexFromString_Parseable, T>::Value == false
		>::Type, typename T2 = void, typename T3 = void>
		T ParseValue(const FString& s)
	{
		static_assert(sizeof(int32) >= sizeof(T), "Cannot parse value because enum class T underlying type is bigger than int32");
		int32 Result;
		if (LexTryParseString(Result, *s))
			return static_cast<T>(Result);
		return T();
	}

	template<typename T, typename EnableType = typename TEnableIf<TIsSame<T, FVector>::Value>::Type>
	FVector ParseValue(const FString& s)
	{
		FVector Result;
		Result.InitFromString(s);
		return Result;
	}
}

/**
 * Utility that allows easy parsing of test parameters for complex automation tests.
 */
struct FAutomationTestParameterParser
{
private:
	const FString ParameterDelimiter;
	const FString ArrayDelimiter;
	TArray<FString> Parameters;
	int32 NumParameters = 0;

public:
	FAutomationTestParameterParser(FString ParametersString, FString InParameterDelimiter, FString InArrayDelimiter) :
		ParameterDelimiter(InParameterDelimiter),
		ArrayDelimiter(InArrayDelimiter)
	{
		NumParameters = ParametersString.ParseIntoArray(Parameters, *ParameterDelimiter);
	}
	
	FAutomationTestParameterParser(FString ParametersString, FString InParameterDelimiter) :
		FAutomationTestParameterParser(ParametersString, InParameterDelimiter, TEXT(";"))
	{
	}

	FAutomationTestParameterParser(FString ParametersString) :
		FAutomationTestParameterParser(ParametersString, TEXT("|"))
	{
	}

	FORCEINLINE int32 GetNumParamters() const
	{
		return NumParameters;
	}

	template<typename T>
	T GetValue(int32 Index) const
	{
		if (!Parameters.IsValidIndex(Index))
		{
			UE_LOG(LogOpenUnrealUtilitiesTests, Error, TEXT("Failed to parse parameter with index %i: Index out of range!"), Index);
			return T();
		}

		return OUUTests_Internal::ParseValue<T>(Parameters[Index]);
	}

	template<typename T>
	TArray<T> GetArrayValue(int32 Index) const
	{
		TArray<T> Result;
		if (!Parameters.IsValidIndex(Index))
		{
			UE_LOG(LogOpenUnrealUtilitiesTests, Error, TEXT("Failed to parse parameter with index %i: Index out of range!"), Index);
			return Result;
		}

		TArray<FString> ParameterArray;
		Parameters[Index].ParseIntoArray(ParameterArray, *ArrayDelimiter);
		for (const FString& s : ParameterArray)
		{
			Result.Add(OUUTests_Internal::ParseValue<T>(s));
		}
		return Result;
	}
};

#endif
