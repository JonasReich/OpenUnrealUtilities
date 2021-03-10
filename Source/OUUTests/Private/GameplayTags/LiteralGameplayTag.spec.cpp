// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "GameplayTags/LiteralGameplayTag.h"

struct FTestGameplayTags : public FLiteralGameplayTagRoot
{
	OUU_GTAG(Player);
	OUU_GTAG_GROUP(NPC)
		OUU_GTAG_GROUP(Mobility)
			OUU_GTAG(Driving);
			OUU_GTAG(OnFoot);
			OUU_GTAG(StandingUp);
		};
	};
};

BEGIN_DEFINE_SPEC(FLiteralGameplayTagSpec, "OpenUnrealUtilities.GameplayTags.LiteralGameplayTag", DEFAULT_OUU_TEST_FLAGS)
END_DEFINE_SPEC(FLiteralGameplayTagSpec)

void FLiteralGameplayTagSpec::Define()
{
	It("should work", [this]()
	{
		FGameplayTag Tag = FTestGameplayTags::NPC::Mobility::Driving::Get();

		SPEC_TEST_EQUAL(Tag.GetTagName(), FName("NPC.Mobility.Driving"));
		FString Name = FTestGameplayTags::NPC::Mobility::Driving::GetName();
		UE_LOG(LogTemp, Log, TEXT("TAG: %s; TAG NAME: %s"), *Tag.GetTagName().ToString(), *Name);

		FGameplayTag PlayerTag = FTestGameplayTags::Player::Get();
		SPEC_TEST_EQUAL(PlayerTag.GetTagName(), FName("Player"));
	});
}

#endif
