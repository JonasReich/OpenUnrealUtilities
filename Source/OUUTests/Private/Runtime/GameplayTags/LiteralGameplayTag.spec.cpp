// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "GameplayTags/SampleGameplayTags.h"
#include "GameplayTags/LiteralGameplayTag.h"

BEGIN_DEFINE_SPEC(FLiteralGameplayTagSpec, "OpenUnrealUtilities.Runtime.GameplayTags.LiteralGameplayTag", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FLiteralGameplayTagSpec)

void FLiteralGameplayTagSpec::Define()
{
	Describe("Get", [this]()
	{
		It("should return the matching tag for root tag groups", [this]()
		{
			const FGameplayTag Tag = FSampleGameplayTags::OUUTestTags::Get();
			SPEC_TEST_EQUAL(Tag.GetTagName(), FName("OUUTestTags"));
		});

		It("should return the matching tag for a simple tag below the root tag group", [this]()
		{
			const FGameplayTag Tag = FSampleGameplayTags::OUUTestTags::Foo::Get();
			SPEC_TEST_EQUAL(Tag.GetTagName(), FName("OUUTestTags.Foo"));
		});

		It("should return the matching tag for nested tags", [this]()
		{
			const FGameplayTag Tag = FSampleGameplayTags::OUUTestTags::Bar::Alpha::Get();
			SPEC_TEST_EQUAL(Tag.GetTagName(), FName("OUUTestTags.Bar.Alpha"));
		});

		It("should return an invalid tag for tags that are not registered", [this]()
		{
			// Disable errors, otherwise we get a nasty ensureAlways from the gameplay tags manager
			constexpr bool bErrorIfNotFound = false;
			const FGameplayTag Tag = FSampleGameplayTags_NotRegsitered::OUUTestTag_NotRegistered::Get(bErrorIfNotFound);
			SPEC_TEST_FALSE(Tag.IsValid());
		});
	});

	Describe("GetName", [this]()
	{
		It("should return a matching name for root tag groups", [this]()
		{
			const FString Name = FSampleGameplayTags::OUUTestTags::GetName();
			SPEC_TEST_EQUAL(Name, FString("OUUTestTags"));
		});

		It("should return a matching name for a simple tag below the root tag group", [this]()
		{
			const FString Name = FSampleGameplayTags::OUUTestTags::Foo::GetName();
			SPEC_TEST_EQUAL(Name, FString("OUUTestTags.Foo"));
		});

		It("should return a matching name for nested tags", [this]()
		{
			const FString Name = FSampleGameplayTags::OUUTestTags::Bar::Alpha::GetName();
			SPEC_TEST_EQUAL(Name, FString("OUUTestTags.Bar.Alpha"));
		});
	});

	Describe("GetRelativeName", [this]()
	{
		It("should return the full name for a root tag", [this]()
		{
			const FString FullName = FSampleGameplayTags::OUUTestTags::GetName();
			const FString RelativeName = FSampleGameplayTags::OUUTestTags::GetRelativeName();
			SPEC_TEST_EQUAL(FullName, FullName);
		});

		It ("should return only the specified highest tag element for nested group tags", [this]()
		{
			const FString Name = FSampleGameplayTags::OUUTestTags::Bar::GetRelativeName();
            SPEC_TEST_EQUAL(Name, FString("Bar"));
		});

		It("should return only the leaf tag element for nested simple tags", [this]()
		{
			const FString Name = FSampleGameplayTags::OUUTestTags::Bar::Alpha::GetRelativeName();
			SPEC_TEST_EQUAL(Name, FString("Alpha"));
		});
	});
}

#endif
