// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "GameplayTags/LiteralGameplayTag.h"
	#include "GameplayTags/SampleGameplayTags.h"

// Static tests for TIsChildTagOf trait
static_assert(TIsChildTagOf<FSampleGameplayTags, FSampleGameplayTags>::Value, "Tag is child of itself");
static_assert(
	TIsChildTagOf<FSampleGameplayTags::Foo, FSampleGameplayTags>::Value,
	"Child tag is child of immediate parent");
static_assert(
	TIsChildTagOf<FSampleGameplayTags::Bar, FSampleGameplayTags>::Value,
	"Second child tag is also child of immediate parent");
static_assert(TIsChildTagOf<FSampleGameplayTags::Bar::Alpha, FSampleGameplayTags>::Value, "Child in child is child");
static_assert(
	TIsChildTagOf<FSampleGameplayTags::Bar::Alpha, FSampleGameplayTags::Foo>::Value == false,
	"Tag in different branch of shared parent is not child");

BEGIN_DEFINE_SPEC(
	FLiteralGameplayTagSpec,
	"OpenUnrealUtilities.Runtime.GameplayTags.LiteralGameplayTag",
	DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FLiteralGameplayTagSpec)

void FLiteralGameplayTagSpec::Define()
{
	Describe("Get", [this]() {
		It("should return the matching tag for root tag groups", [this]() {
			const FGameplayTag Tag = FSampleGameplayTags::Get();
			SPEC_TEST_EQUAL(Tag.GetTagName(), FName("OUUTestTags"));
		});

		It("should return the matching tag for a simple tag below the root tag group", [this]() {
			const FGameplayTag Tag = FSampleGameplayTags::Foo::Get();
			SPEC_TEST_EQUAL(Tag.GetTagName(), FName("OUUTestTags.Foo"));
		});

		It("should return the matching tag for nested tags", [this]() {
			const FGameplayTag Tag = FSampleGameplayTags::Bar::Alpha::Get();
			SPEC_TEST_EQUAL(Tag.GetTagName(), FName("OUUTestTags.Bar.Alpha"));
		});

		It("should return an invalid tag for tags that are not registered", [this]() {
			// Disable errors, otherwise we get a nasty ensureAlways from the gameplay tags manager
			AddExpectedError("Requested Gameplay Tag UUTestTag_NotRegistered was not found");
			const FGameplayTag Tag = FSampleGameplayTags_NotRegsitered::GetTag();
			SPEC_TEST_FALSE(Tag.IsValid());
		});
	});

	Describe("GetName", [this]() {
		It("should return a matching name for root tag groups", [this]() {
			const FString Name = FSampleGameplayTags::GetName();
			SPEC_TEST_EQUAL(Name, FString("OUUTestTags"));
		});

		It("should return a matching name for a simple tag below the root tag group", [this]() {
			const FString Name = FSampleGameplayTags::Foo::GetName();
			SPEC_TEST_EQUAL(Name, FString("OUUTestTags.Foo"));
		});

		It("should return a matching name for nested tags", [this]() {
			const FString Name = FSampleGameplayTags::Bar::Alpha::GetName();
			SPEC_TEST_EQUAL(Name, FString("OUUTestTags.Bar.Alpha"));
		});
	});

	Describe("GetRelativeName", [this]() {
		It("should return the full name for a root tag", [this]() {
			const FString FullName = FSampleGameplayTags::GetName();
			const FString RelativeName = FSampleGameplayTags::GetRelativeName();
			SPEC_TEST_EQUAL(FullName, FullName);
		});

		It("should return only the specified highest tag element for nested group tags", [this]() {
			const FString Name = FSampleGameplayTags::Bar::GetRelativeName();
			SPEC_TEST_EQUAL(Name, FString("Bar"));
		});

		It("should return only the leaf tag element for nested simple tags", [this]() {
			const FString Name = FSampleGameplayTags::Bar::Alpha::GetRelativeName();
			SPEC_TEST_EQUAL(Name, FString("Alpha"));
		});
	});
}

#endif
