// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUMacros.h"
#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER
	#include "GameplayTags/GameplayTagDependencies.h"
	#include "GameplayTags/SampleGameplayTags.h"
	#include "GameplayTagDependenciesTests.h"

BEGIN_DEFINE_SPEC(
	FTagDependenciesSpec,
	"OpenUnrealUtilities.Runtime.GameplayTags.TagDependencies",
	DEFAULT_OUU_TEST_FLAGS)
	UGameplayTagDependency_TestObject* ObjectA;
	UGameplayTagDependency_TestObject* ObjectB; // depends on A
	UGameplayTagDependency_TestObject* ObjectB2;
	UGameplayTagDependency_TestObject* ObjectC; // depends on B + B2
	// -- for event tests only
	UGameplayTagDependency_TestEventHandler* EventHandler;
END_DEFINE_SPEC(FTagDependenciesSpec)

void FTagDependenciesSpec::Define()
{
	BeforeEach([this]() {
		ObjectA = NewObject<UGameplayTagDependency_TestObject>();
		ObjectB = NewObject<UGameplayTagDependency_TestObject>();
		ObjectB2 = NewObject<UGameplayTagDependency_TestObject>();
		ObjectC = NewObject<UGameplayTagDependency_TestObject>();

		ObjectB->AddDependency(ObjectA);
		ObjectC->AddDependency(ObjectB);
		ObjectC->AddDependency(ObjectB2);
	});

	Describe("BroadcastTagsChanged", [this]() {
		It("should allow propagating tag changes", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();
			FGameplayTagContainer Tags;
			ObjectC->AppendTags(OUT Tags);
			SPEC_TEST_TRUE(Tags.Num() > 0);
		});

		It("should not have any effects if a dependant becomes invalid", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectB->MarkAsGarbage();
			ObjectA->BroadcastTagsChanged();
			FGameplayTagContainer Tags;
			ObjectC->AppendTags(OUT Tags);
			SPEC_TEST_TRUE(Tags.Num() == 0);
		});

		It("should allow combining tags from multiple sources", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();
			ObjectB->SourceContainer.AddTag(FSampleGameplayTags::Bar::Get());
			ObjectB->BroadcastTagsChanged();
			ObjectB2->SourceContainer.AddTag(FSampleGameplayTags::Baz::Get());
			ObjectB2->BroadcastTagsChanged();
			FGameplayTagContainer Tags;
			ObjectC->AppendTags(OUT Tags);
			SPEC_TEST_EQUAL(Tags.Num(), 3);
			SPEC_TEST_TRUE(Tags.HasTag(FSampleGameplayTags::Foo::Get()));
			SPEC_TEST_TRUE(Tags.HasTag(FSampleGameplayTags::Bar::Get()));
			SPEC_TEST_TRUE(Tags.HasTag(FSampleGameplayTags::Baz::Get()));
		});

		It("should communicate removed tags", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();
			ObjectB->SourceContainer.AddTag(FSampleGameplayTags::Bar::Get());
			ObjectB->BroadcastTagsChanged();
			ObjectB->SourceContainer.RemoveTag(FSampleGameplayTags::Bar::Get());
			ObjectB->BroadcastTagsChanged();

			FGameplayTagContainer Tags;
			ObjectC->AppendTags(OUT Tags);
			SPEC_TEST_EQUAL(Tags.Num(), 1);
			SPEC_TEST_TRUE(Tags.HasTag(FSampleGameplayTags::Foo::Get()));
		});

		It("should not remove tag from ALL if it was provided by another source", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();
			ObjectB->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectB->BroadcastTagsChanged();

			ObjectA->SourceContainer.RemoveTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();

			FGameplayTagContainer Tags;
			ObjectC->AppendTags(OUT Tags);
			SPEC_TEST_EQUAL(Tags.Num(), 1);
			SPEC_TEST_TRUE(Tags.HasTag(FSampleGameplayTags::Foo::Get()));
		});
	});

	Describe("OnTagsChanged", [this]() {
		BeforeEach([this]() {
			EventHandler = NewObject<UGameplayTagDependency_TestEventHandler>();
			const auto Event = CreateDynamic(
				FGameplayTagDependencyEvent,
				EventHandler,
				&UGameplayTagDependency_TestEventHandler::HandleOnTagsChanged);
			ObjectC->BindEventToOnTagsChanged(Event);
		});

		It("should pass added, removed, and current tags", [this]() {
			// add tags + broadcast
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Bar::Get());
			ObjectA->BroadcastTagsChanged();

			SPEC_TEST_EQUAL(EventHandler->NumDelegateCalled, 1);
			SPEC_TEST_EQUAL(
				EventHandler->LastChange.AllTags,
				FGameplayTagContainer::CreateFromArray(
					TArray<FGameplayTag>{FSampleGameplayTags::Foo::Get(), FSampleGameplayTags::Bar::Get()}));
			SPEC_TEST_EQUAL(
				EventHandler->LastChange.NewTags,
				FGameplayTagContainer::CreateFromArray(
					TArray<FGameplayTag>{FSampleGameplayTags::Foo::Get(), FSampleGameplayTags::Bar::Get()}));
			SPEC_TEST_EQUAL(EventHandler->LastChange.RemovedTags, FGameplayTagContainer());

			// add + remove tags + broadcast
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Baz::Get());
			ObjectA->SourceContainer.RemoveTag(FSampleGameplayTags::Bar::Get());
			ObjectA->BroadcastTagsChanged();

			SPEC_TEST_EQUAL(EventHandler->NumDelegateCalled, 2);
			SPEC_TEST_EQUAL(
				EventHandler->LastChange.AllTags,
				FGameplayTagContainer::CreateFromArray(
					TArray<FGameplayTag>{FSampleGameplayTags::Foo::Get(), FSampleGameplayTags::Baz::Get()}));
			SPEC_TEST_EQUAL(EventHandler->LastChange.NewTags, FGameplayTagContainer(FSampleGameplayTags::Baz::Get()));
			SPEC_TEST_EQUAL(
				EventHandler->LastChange.RemovedTags,
				FGameplayTagContainer(FSampleGameplayTags::Bar::Get()));

			// call a third time -> should not result in new delegate call, because there is no change
			ObjectA->BroadcastTagsChanged();

			SPEC_TEST_EQUAL(EventHandler->NumDelegateCalled, 2);
		});

		It("should not be called if tags do not change from perspective of target object", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();
			SPEC_TEST_EQUAL(EventHandler->NumDelegateCalled, 1);

			ObjectB->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectB->BroadcastTagsChanged();
			ObjectA->SourceContainer.RemoveTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();

			// should be only 1, because from POV of C the tags only changed once at beginning
			SPEC_TEST_EQUAL(EventHandler->NumDelegateCalled, 1);
		});

		It("should not be called if object becomes invalid", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			EventHandler->MarkAsGarbage();
			SPEC_TEST_FALSE(IsValid(EventHandler));
			ObjectA->BroadcastTagsChanged();
			SPEC_TEST_EQUAL(EventHandler->NumDelegateCalled, 0);
		});
	});

	Describe("GetImmediateTagSources", [this]() {
		It("should return own or immediate dependencies containing tags", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();
			ObjectB->SourceContainer.AddTag(FSampleGameplayTags::Bar::Get());
			ObjectB->BroadcastTagsChanged();
			ObjectC->SourceContainer.AddTag(FSampleGameplayTags::Baz::Get());
			ObjectC->BroadcastTagsChanged();

			TMap<FGameplayTag, const UObject*> ExpectedResult;
			ExpectedResult.Add(FSampleGameplayTags::Foo::Get(), ObjectB); // only go 1 level up
			ExpectedResult.Add(FSampleGameplayTags::Bar::Get(), ObjectB);
			ExpectedResult.Add(FSampleGameplayTags::Baz::Get(), ObjectC);

			const auto ActualResult = ObjectC->GetImmediateTagSources();
			SPEC_TEST_TRUE(ActualResult.OrderIndependentCompareEqual(ExpectedResult));
		});
	});

	Describe("GetOriginalTagSources", [this]() {
		It("should return sources from all along the hierarchy", [this]() {
			ObjectA->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectA->BroadcastTagsChanged();
			ObjectB->SourceContainer.AddTag(FSampleGameplayTags::Foo::Get());
			ObjectB->SourceContainer.AddTag(FSampleGameplayTags::Bar::Get());
			ObjectB->BroadcastTagsChanged();
			ObjectC->SourceContainer.AddTag(FSampleGameplayTags::Baz::Get());
			ObjectC->BroadcastTagsChanged();

			TMap<FGameplayTag, TSet<const UObject*>> ExpectedResult, ActualResult;
			ExpectedResult.Add(FSampleGameplayTags::Foo::Get(), {ObjectA, ObjectB});
			ExpectedResult.Add(FSampleGameplayTags::Bar::Get(), {ObjectB});
			ExpectedResult.Add(FSampleGameplayTags::Baz::Get(), {ObjectC});

			ObjectC->GetOriginalTagSources(OUT ActualResult);
			for (auto& ExpectedEntry : ExpectedResult)
			{
				auto& ExpectedObjectSet = ExpectedEntry.Value;
				if (const auto* ActualObjectSet = ActualResult.Find(ExpectedEntry.Key))
				{
					SPEC_TEST_TRUE(ActualObjectSet->Intersect(ExpectedObjectSet).Num() == ExpectedObjectSet.Num());
				}
				else
				{
					TestTrue("Expected tag not found", false);
				}
			}
		});
	});
}

#endif
