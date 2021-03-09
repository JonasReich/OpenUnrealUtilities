// Copyright (c) 2021 Jonas Reich

#pragma once

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

enum class EParameterParserTestEnum
{
	Alpha,
	Beta,
	Gamma
};

enum class EParameterParserTestEnum_Parseable
{
	Alpha,
	Beta,
	Gamma,
	Invalid
};

void LexFromString(EParameterParserTestEnum_Parseable& Out, FString S)
{
	if (S == "Alpha") 
	{
		Out = EParameterParserTestEnum_Parseable::Alpha;
	}
	else if (S == "Beta")
	{
		Out = EParameterParserTestEnum_Parseable::Beta;
	}
	else if (S == "Gamma")
	{
		Out = EParameterParserTestEnum_Parseable::Gamma;
	}
	else
	{
		Out = EParameterParserTestEnum_Parseable::Invalid;
	}
}

BEGIN_DEFINE_SPEC(FAutomationTestParameterParserSpec, "OpenUnrealUtilities.TestUtilities.ParameterParser", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FAutomationTestParameterParserSpec)
void FAutomationTestParameterParserSpec::Define()
{
	Describe("GetNumParamters", [this]() 
	{
		It("should return the number of parameters contained in the parameter string that were separated by '|'", [this]()
		{
			FAutomationTestParameterParser Parser("1|2|3|4|5");
			TestEqual("Number of parameters", Parser.GetNumParamters(), 5);
		});
	});

	Describe("GetValue", [this]() 
	{
		It("should parse parameter values based on the index", [this]()
		{
			FAutomationTestParameterParser Parser("0|1|2|42");
			TestEqual("Parameter #1", Parser.GetValue<int32>(0), 0);
			TestEqual("Parameter #2", Parser.GetValue<int32>(1), 1);
			TestEqual("Parameter #3", Parser.GetValue<int32>(2), 2);
			TestEqual("Parameter #4", Parser.GetValue<int32>(3), 42);
		});
		
		It("should throw an error without crashing if the index is out of range", [this]() 
		{
			AddExpectedError("Index out of range", EAutomationExpectedErrorFlags::Contains, 1);
			FAutomationTestParameterParser Parser("0|1");
			Parser.GetValue<int32>(2);
		});

		It("should split parameters using a custom delimiter if specified in constructor", [this]()
		{
			FAutomationTestParameterParser Parser("0OUU1OUU2", "OUU");
			TestEqual("Parameter #1", Parser.GetValue<int32>(0), 0);
			TestEqual("Parameter #2", Parser.GetValue<int32>(1), 1);
			TestEqual("Parameter #3", Parser.GetValue<int32>(2), 2);
		});

		It("should parse string parameters", [this]()
		{
			FAutomationTestParameterParser Parser("0|My test string|2");
			TestEqual("Result string", Parser.GetValue<FString>(1), "My test string");
		});

		It("should parse boolean parameters", [this]()
		{
			FAutomationTestParameterParser Parser("0|1|true|false");
			TestEqual("Parameter #1", Parser.GetValue<bool>(0), false);
			TestEqual("Parameter #2", Parser.GetValue<bool>(1), true);
			TestEqual("Parameter #3", Parser.GetValue<bool>(2), true);
			TestEqual("Parameter #4", Parser.GetValue<bool>(3), false);
		});

		It("should parse enum parameters from integers if LexFromString is NOT defined", [this]()
		{
			FAutomationTestParameterParser Parser("1|0|2");
			TestEqual("Parameter #1", Parser.GetValue<EParameterParserTestEnum>(0), EParameterParserTestEnum::Beta);
			TestEqual("Parameter #2", Parser.GetValue<EParameterParserTestEnum>(1), EParameterParserTestEnum::Alpha);
			TestEqual("Parameter #3", Parser.GetValue<EParameterParserTestEnum>(2), EParameterParserTestEnum::Gamma);
		});

		It("should parse enum parameters from strings if LexFromString IS defined", [this]()
		{
			FAutomationTestParameterParser Parser("Beta|Alpha|0|Gamma");
			TestEqual("Parameter #1", Parser.GetValue<EParameterParserTestEnum_Parseable>(0), EParameterParserTestEnum_Parseable::Beta);
			TestEqual("Parameter #2", Parser.GetValue<EParameterParserTestEnum_Parseable>(1), EParameterParserTestEnum_Parseable::Alpha);
			TestEqual("Parameter #3", Parser.GetValue<EParameterParserTestEnum_Parseable>(2), EParameterParserTestEnum_Parseable::Invalid);
			TestEqual("Parameter #4", Parser.GetValue<EParameterParserTestEnum_Parseable>(3), EParameterParserTestEnum_Parseable::Gamma);
		});
	});

	Describe("GetArrayValue", [this]() 
	{
		It("should return an array of elements that were separated by ';'", [this]() 
		{
			FAutomationTestParameterParser Parser("1;2;3|4;5;6|7;8;9");
			TestEqual("Result array", Parser.GetArrayValue<int32>(1), { 4, 5, 6 });
		});

		It("should return an array of elements that were separated by custom delimiter if specified in constructor", [this]()
		{
			FAutomationTestParameterParser Parser("1@2@3o4@5@6o7@8@9", "o", "@");
			TestEqual("Result array", Parser.GetArrayValue<int32>(2), { 7, 8, 9 });
		});
	});
}

#endif
