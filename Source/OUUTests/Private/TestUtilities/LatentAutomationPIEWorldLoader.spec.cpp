// Copyright (c) 2023 Jonas Reich & Contributors

#include "LatentAutomationPIEWorldLoader.h"

#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

BEGIN_DEFINE_SPEC(
	FLatentAutomationPIEWorldLoaderSpec,
	"OpenUnrealUtilities.TestUtilities.LatentAutomationPIEWorldLoader",
	DEFAULT_OUU_TEST_FLAGS)
	OUU::TestUtilities::FLatentAutomationPIEWorldLoader WorldLoader{*this};
END_DEFINE_SPEC(FLatentAutomationPIEWorldLoaderSpec)
void FLatentAutomationPIEWorldLoaderSpec::Define()
{
	Describe("LatentLoad", [this]() {
		LatentBeforeEach([this](const FDoneDelegate& Done) { WorldLoader.LatentLoad(Done); });
		It("should load a valid OUU-empty-world map", [this]() {
			auto* LoadedWorld = WorldLoader.GetLoadedWorld();
			if (!SPEC_TEST_NOT_NULL(LoadedWorld))
				return;

			SPEC_TEST_EQUAL(LoadedWorld->GetName(), FString(TEXT("EmptyWorld")));
		});

		LatentIt("should reload the world and clean previously spawned actors", [this](const FDoneDelegate& Done) {
			auto* LoadedWorld = WorldLoader.GetLoadedWorld();
			if (!SPEC_TEST_NOT_NULL(LoadedWorld))
			{
				Done.Execute();
				return;
			}

			const int32 OriginalActorCount = LoadedWorld->GetActorCount();
			auto* MeshActor = LoadedWorld->SpawnActor<AStaticMeshActor>();
			const int32 ActorCountAfterSpawn = LoadedWorld->GetActorCount();

			SPEC_TEST_NOT_NULL(MeshActor);
			if (!SPEC_TEST_EQUAL(OriginalActorCount + 1, ActorCountAfterSpawn))
			{
				Done.Execute();
				return;
			}

			WorldLoader.LatentLoad(FDoneDelegate::CreateLambda([this, OriginalActorCount, Done]() {
				auto* Loaded2ndWorld = WorldLoader.GetLoadedWorld();
				if (!SPEC_TEST_NOT_NULL(Loaded2ndWorld))
				{
					Done.Execute();
					return;
				}

				const int32 ActorCountInSecondWorld = Loaded2ndWorld->GetActorCount();
				SPEC_TEST_EQUAL(OriginalActorCount, ActorCountInSecondWorld);

				Done.Execute();
			}));
		});
	});

	Describe("LatentLoadBeforeEach", [this]() {
		// This is specifically meant to be used in-place of an explicit BeforeEach block!
		WorldLoader.LatentLoadBeforeEach();

		It("should load a valid OUU-empty-world map", [this]() {
			auto* LoadedWorld = WorldLoader.GetLoadedWorld();
			if (!SPEC_TEST_NOT_NULL(LoadedWorld))
				return;

			SPEC_TEST_EQUAL(LoadedWorld->GetName(), FString(TEXT("EmptyWorld")));
		});
	});

	AfterEach([this]() { WorldLoader.ClosePIE(); });
}

#endif
