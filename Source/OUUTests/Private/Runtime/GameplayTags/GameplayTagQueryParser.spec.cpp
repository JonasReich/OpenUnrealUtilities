// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"
#include "GameplayTags/SampleGameplayTags.h"

#if WITH_AUTOMATION_WORKER

#include "GameplayTags/GameplayTagQueryParser.h"

FGameplayTagContainer TagContainerFromStrings(std::initializer_list<FString> TagStrings)
{
	FGameplayTagContainer Result{};
	for (const FString& TagString : TagStrings)
	{
		Result.AddTag(FGameplayTag::RequestGameplayTag(*TagString));
	}
	return Result;
}


BEGIN_DEFINE_SPEC(FGameplayTagQueryParserSpec, "OpenUnrealUtilities.Runtime.GameplayTags.GameplayTagQueryParser", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FGameplayTagQueryParserSpec)

void FGameplayTagQueryParserSpec::Define()
{
	Describe("ParseQuery", [this]()
	{
		It("should create a query from a single tag without any operator", [this]()
		{
			auto Query = FGameplayTagQueryParser::ParseQuery("OUUTestTags.Foo");
			TestTrue("Query matches [OUUTestTags.Foo]", Query.Matches(FGameplayTagContainer(FSampleGameplayTags::OUUTestTags::Foo::Get())));
		});

		It("should create a query from a single ANY tag query", [this]()
		{
			auto Query = FGameplayTagQueryParser::ParseQuery("ANY(OUUTestTags.Foo)");
			TestTrue("Query matches [OUUTestTags.Foo]", Query.Matches(FGameplayTagContainer(FSampleGameplayTags::OUUTestTags::Foo::Get())));
		});

		It("should create a query that behaves as expected with nested queries", [this]()
		{
			auto Query = FGameplayTagQueryParser::ParseQuery("ANY(ALL(OUUTestTags.Foo, OUUTestTags.Bar.Alpha), ANY(OUUTestTags.Bar.Beta, OUUTestTags.Bar.Gamma))");

			// Single tags not sufficient
			TestFalse("Query matches [OUUTestTags.Foo]", Query.Matches(TagContainerFromStrings({FString("OUUTestTags.Foo")})));
			TestFalse("Query matches [OUUTestTags.Bar.Alpha]", Query.Matches(TagContainerFromStrings({FString("OUUTestTags.Bar.Alpha")})));
			// Multiple tags okay
			TestTrue("Query matches [OUUTestTags.Foo, OUUTestTags.Bar.Alpha]", Query.Matches(TagContainerFromStrings({FString("OUUTestTags.Foo"), FString("OUUTestTags.Bar.Alpha")})));
			// Tag not present
			TestFalse("Query matches [OUUTestTags.Bar.Delta]", Query.Matches(TagContainerFromStrings({FString("OUUTestTags.Bar.Delta")})));
			// Single tags okay
			TestTrue("Query matches [OUUTestTags.Bar.Beta]", Query.Matches(TagContainerFromStrings({FString("OUUTestTags.Bar.Beta")})));
			TestTrue("Query matches [OUUTestTags.Bar.Gamma]", Query.Matches(TagContainerFromStrings({FString("OUUTestTags.Bar.Gamma")})));
			// One not okay, the other is
			TestTrue("Query matches [OUUTestTags.Foo, OUUTestTags.Bar.Gamma]", Query.Matches(TagContainerFromStrings({FString("OUUTestTags.Foo"), FString("OUUTestTags.Bar.Gamma")})));
		});
	});
}

#endif
