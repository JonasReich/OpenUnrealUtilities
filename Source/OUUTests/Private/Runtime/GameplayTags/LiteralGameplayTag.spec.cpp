// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "GameplayTags/LiteralGameplayTag.h"

/**
 * Literal gameplay tag hierarchy used for FLiteralGameplayTagSpec.
 *
 * Note that the tags declared here are most likely not part of your projects GameplayTags.ini configuration.
 * However, without the tags being present in the actual project configuration the gameplay tags manager will
 * not be able to locate them and throw an ensure.
 * Mandating those tags to be present would severely limit the portability of the Open Unreal Utilities plugin.
 *
 * Therefore, the spec below does not test the Get() function that returns actual gameplay tags and instead only
 * tests the underlying functions for name composition.
 */
struct FTestGameplayTags : public FLiteralGameplayTagRoot
{
	OUU_GTAG(Foo);
	OUU_GTAG_GROUP(Bar)
		OUU_GTAG_GROUP(Letter)
			OUU_GTAG(Alpha);
			OUU_GTAG(Beta);
			OUU_GTAG(Gamma);
		};
	};
};

BEGIN_DEFINE_SPEC(FLiteralGameplayTagSpec, "OpenUnrealUtilities.GameplayTags.LiteralGameplayTag", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FLiteralGameplayTagSpec)

void FLiteralGameplayTagSpec::Define()
{
	Describe("GetName", [this]()
	{
		It("should return a matching name for simple tags", [this]()
		{
			const FString Name = FTestGameplayTags::Foo::GetName();
			SPEC_TEST_EQUAL(Name, FString("Foo"));
		});

		It("should return a matching name for nested tags", [this]()
		{
			const FString Name = FTestGameplayTags::Bar::Letter::Alpha::GetName();
			SPEC_TEST_EQUAL(Name, FString("Bar.Letter.Alpha"));
		});
	});

	Describe("GetRelativeName", [this]()
	{
		It("should return the full name for simple tags", [this]()
		{
			const FString FullName = FTestGameplayTags::Foo::GetName();
			const FString RelativeName = FTestGameplayTags::Foo::GetRelativeName();
			SPEC_TEST_EQUAL(FullName, FullName);
		});

		It ("should return only the specified highest tag element for nested container tags", [this]()
		{
			const FString Name = FTestGameplayTags::Bar::Letter::GetRelativeName();
            SPEC_TEST_EQUAL(Name, FString("Letter"));
		});

		It("should return only the leaf tag element for nested simple tags", [this]()
		{
			const FString Name = FTestGameplayTags::Bar::Letter::Alpha::GetRelativeName();
			SPEC_TEST_EQUAL(Name, FString("Alpha"));
		});
	});
}

#endif
