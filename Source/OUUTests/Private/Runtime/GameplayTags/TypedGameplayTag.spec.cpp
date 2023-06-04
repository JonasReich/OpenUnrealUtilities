// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "GameplayTags/TypedGameplayTag.h"
	#include "GameplayTags/SampleGameplayTags.h"

PRAGMA_DISABLE_OPTIMIZATION

static_assert(
	!std::is_assignable<FOUUSampleBarTag, FGameplayTag>::value,
	"Typed tags must never be assignable from a regular gameplay tag");

static_assert(
	TIsConstructible<FOUUSampleBarTags_Ref>::Value == false,
	"Typed tags container ref type should not be constructible without parameters");
static_assert(
	TIsConstructible<FOUUSampleBarTags_Ref, FOUUSampleBarTags_Value>::Value == true,
	"Typed tags container ref type should be constructible from value type");

BEGIN_DEFINE_SPEC(
	FTypedGameplayTagSpec,
	"OpenUnrealUtilities.Runtime.GameplayTags.TypedGameplayTag",
	DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FTypedGameplayTagSpec)

void FTypedGameplayTagSpec::Define()
{
	It("should be constructible from a literal gameplay tag", [this]() {
		FOUUSampleBarTag InstanceTag{FSampleGameplayTags::Bar::Get()};
		FGameplayTag RegularTag{FSampleGameplayTags::Bar::Get()};
		SPEC_TEST_EQUAL(InstanceTag.ToString(), RegularTag.ToString());
	});

	It("should be assignable from a literal gameplay tag", [this]() {
		FOUUSampleBarTag InstanceTag;
		FGameplayTag RegularTag;
		InstanceTag = FSampleGameplayTags::Bar::Get();
		RegularTag = FSampleGameplayTags::Bar::Get();
		SPEC_TEST_EQUAL(InstanceTag.ToString(), RegularTag.ToString());
	});

	Describe("operator==", [this]() {
		It("should support comparison with a regular gameplay tag", [this]() {
			FOUUSampleBarTag TypedTag = FSampleGameplayTags::Bar::Get();
			FGameplayTag RegularTag = FSampleGameplayTags::Bar::Get();
			FGameplayTag RegularTag_Foo = FSampleGameplayTags::Foo::Get();

			SPEC_TEST_TRUE(TypedTag == RegularTag);
			SPEC_TEST_TRUE(RegularTag == TypedTag);

			SPEC_TEST_FALSE(TypedTag == RegularTag_Foo);
			SPEC_TEST_FALSE(RegularTag_Foo == TypedTag);
		});

		It("should support comparison with a literal gameplay tag", [this]() {
			const FSampleGameplayTags::Foo& LiteralTag_Foo = FSampleGameplayTags::Foo::Get();
			const FSampleGameplayTags::Bar& LiteralTag_Bar = FSampleGameplayTags::Bar::Get();
			FOUUSampleBarTag TypedTag_Bar = LiteralTag_Bar;

			SPEC_TEST_FALSE(TypedTag_Bar == LiteralTag_Foo);
			SPEC_TEST_FALSE(LiteralTag_Foo == TypedTag_Bar);

			SPEC_TEST_TRUE(TypedTag_Bar == LiteralTag_Bar);
			SPEC_TEST_TRUE(LiteralTag_Bar == TypedTag_Bar);
		});

		It("should support comparison with another typed tag", [this]() {
			FOUUSampleBarTag TypedTag_Bar_1 = FSampleGameplayTags::Bar::Get();
			FOUUSampleBarTag TypedTag_Bar_2 = FSampleGameplayTags::Bar::Get();
			FOUUSampleBarTag TypedTag_Bar_Alpha = FSampleGameplayTags::Bar::Alpha::Get();

			SPEC_TEST_TRUE(TypedTag_Bar_1 == TypedTag_Bar_2);

			SPEC_TEST_FALSE(TypedTag_Bar_1 == TypedTag_Bar_Alpha);
			SPEC_TEST_FALSE(TypedTag_Bar_Alpha == TypedTag_Bar_1);
		});
	});

	Describe("TryConvert", [this]() {
		It("should succeed with matching tags", [this]() {
			{
				FGameplayTag VanillaBarTag = FGameplayTag::RequestGameplayTag(TEXT("OUUTestTags.Bar"));
				FOUUSampleBarTag Result = FOUUSampleBarTag::TryConvert(VanillaBarTag);
				SPEC_TEST_TRUE(VanillaBarTag.IsValid());
				SPEC_TEST_TRUE(Result.IsValid());
			}
			// Added the option to have two root tags, so we must test this
			{
				FGameplayTag VanillaBazTag = FGameplayTag::RequestGameplayTag(TEXT("OUUTestTags.Baz"));
				FOUUSampleBarTag Result = FOUUSampleBarTag::TryConvert(VanillaBazTag);
				SPEC_TEST_TRUE(VanillaBazTag.IsValid());
				SPEC_TEST_TRUE(Result.IsValid());
			}
		});

		It("should fail with non-matching tags", [this]() {
			FGameplayTag VanillaFooTag = FGameplayTag::RequestGameplayTag(TEXT("OUUTestTags.Foo"));
			// Vanilla tag should always be valid.
			SPEC_TEST_TRUE(VanillaFooTag.IsValid());

			FOUUSampleBarTag Result = FOUUSampleBarTag::TryConvert(VanillaFooTag);
			SPEC_TEST_FALSE(Result.IsValid());
		});
	});

	Describe("GetAllRootTags", [this]() {
		It("should return all native tags", [this]() {
			auto AllRootTags = FOUUSampleBarTag::GetAllRootTags();
			SPEC_TEST_TRUE(AllRootTags.HasTagExact(FSampleGameplayTags::Bar::Get()));
			SPEC_TEST_TRUE(AllRootTags.HasTagExact(FSampleGameplayTags::Baz::Get()));
		});
	});

	Describe("GetAllLeafTags", [this]() {
		It("should return all native leaf tags", [this]() {
			auto AllLeafTags = FOUUSampleBarTag::GetAllLeafTags();

			// These are all children of Bar that don't have any successive children
			SPEC_TEST_TRUE(AllLeafTags.HasTagExact(FSampleGameplayTags::Bar::Alpha::One::Get()));
			SPEC_TEST_TRUE(AllLeafTags.HasTagExact(FSampleGameplayTags::Bar::Alpha::Two::Get()));
			SPEC_TEST_TRUE(AllLeafTags.HasTagExact(FSampleGameplayTags::Bar::Beta::Get()));
			SPEC_TEST_TRUE(AllLeafTags.HasTagExact(FSampleGameplayTags::Bar::Gamma::Get()));
			SPEC_TEST_TRUE(AllLeafTags.HasTagExact(FSampleGameplayTags::Bar::Delta::Get()));

			// this root tag does not have any children, but should still be included.
			SPEC_TEST_TRUE(AllLeafTags.HasTagExact(FSampleGameplayTags::Baz::Get()));
		});

		It("should not return native container tags", [this]() {
			auto AllLeafTags = FOUUSampleBarTag::GetAllLeafTags();
			SPEC_TEST_FALSE(AllLeafTags.HasTagExact(FSampleGameplayTags::Bar::Alpha::Get()));
		});
	});

	Describe("Implicitly typed tag containers", [this]() {
		It("should allow adding matching native tags", [this]() {
			FOUUSampleBarTags_Value Tags;
			Tags.AddTag(FSampleGameplayTags::Bar::Get());
			Tags.AddTag(FSampleGameplayTags::Baz::Get());
			SPEC_TEST_TRUE(Tags.HasTagExact(FSampleGameplayTags::Bar::Get()));
			SPEC_TEST_TRUE(Tags.HasTagExact(FSampleGameplayTags::Baz::Get()));
		});

		It("should be convertible to a regular tag container", [this]() {
			FOUUSampleBarTags_Value Tags;
			Tags.AddTag(FSampleGameplayTags::Bar::Get());
			Tags.AddTag(FSampleGameplayTags::Baz::Get());

			FGameplayTagContainer NormalTags = Tags.Get();
			SPEC_TEST_TRUE(NormalTags.HasTagExact(FSampleGameplayTags::Bar::Get()));
			SPEC_TEST_TRUE(NormalTags.HasTagExact(FSampleGameplayTags::Baz::Get()));
		});

		It("should be convertible from a regular tag container", [this]() {
			FGameplayTagContainer NormalTags;
			NormalTags.AddTag(FSampleGameplayTags::Bar::Get());
			NormalTags.AddTag(FSampleGameplayTags::Baz::Get());

			FOUUSampleBarTags_Value Tags = FOUUSampleBarTags_Value::CreateFiltered(NormalTags);
			SPEC_TEST_TRUE(Tags.HasTagExact(FSampleGameplayTags::Bar::Get()));
			SPEC_TEST_TRUE(Tags.HasTagExact(FSampleGameplayTags::Baz::Get()));
			SPEC_TEST_EQUAL(Tags.Num(), 2);
		});

		It("should filter out unrelated tags during conversion", [this]() {
			FGameplayTagContainer NormalTags;
			NormalTags.AddTag(FSampleGameplayTags::Foo::Get());
			NormalTags.AddTag(FSampleGameplayTags::Bar::Get());

			FOUUSampleBarTags_Value Tags = FOUUSampleBarTags_Value::CreateFiltered(NormalTags);
			SPEC_TEST_TRUE(Tags.HasTagExact(FSampleGameplayTags::Bar::Get()));
			SPEC_TEST_EQUAL(Tags.Num(), 1);
		});

		It("should allow modifying original contianer through ref type", [this]() {
			FOUUSampleBarTags_Value Tags;

			FOUUSampleBarTags_Ref TagsRef = Tags;
			TagsRef.AddTag(FSampleGameplayTags::Bar::Get());

			SPEC_TEST_TRUE(Tags.HasTagExact(FSampleGameplayTags::Bar::Get()));
		});

		It("should support ranged for loops", [this]() {
			FOUUSampleBarTags_Value Tags;
			Tags.AddTag(FSampleGameplayTags::Bar::Get());
			Tags.AddTag(FSampleGameplayTags::Baz::Get());
			Tags.AddTag(FSampleGameplayTags::Bar::Alpha::Get());

			TArray<FGameplayTag> TagsFromRangedForLoop;
			for (auto Tag : Tags)
			{
				TagsFromRangedForLoop.Add(Tag);
			}

			if (SPEC_TEST_TRUE(TagsFromRangedForLoop.Num() == 3))
			{
				SPEC_TEST_EQUAL(TagsFromRangedForLoop[0], FSampleGameplayTags::Bar::GetTag());
				SPEC_TEST_EQUAL(TagsFromRangedForLoop[1], FSampleGameplayTags::Baz::GetTag());
				SPEC_TEST_EQUAL(TagsFromRangedForLoop[2], FSampleGameplayTags::Bar::Alpha::GetTag());
			}
		});
	});
}

PRAGMA_ENABLE_OPTIMIZATION

#endif
