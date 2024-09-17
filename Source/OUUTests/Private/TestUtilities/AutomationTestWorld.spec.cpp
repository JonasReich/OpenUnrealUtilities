// Copyright (c) 2023 Jonas Reich & Contributors

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "AutomationTestWorld.h"
	#include "Engine/GameInstance.h"
	#include "GameFramework/GameModeBase.h"

	#define SPEC_BASE_NAME "OpenUnrealUtilities.TestUtilities.AutomationTestWorld"

BEGIN_DEFINE_SPEC(FAutomationTestWorldSpec, SPEC_BASE_NAME, DEFAULT_OUU_TEST_FLAGS)
	// Worlds often times come with too much baggage (subsystems) to allow reacting to errors
	bool SuppressLogs() override { return true; }

	TSharedPtr<FOUUAutomationTestWorld> TestWorld;
END_DEFINE_SPEC(FAutomationTestWorldSpec)
void FAutomationTestWorldSpec::Define()
{
	BeforeEach([this]() { TestWorld = MakeShared<FOUUAutomationTestWorld>("FAutomationTestWorldSpec"); });

	Describe("", [this]() {
		Describe("CreateWorld", [this]() {
			It("should create a valid test world", [this]() {
				TestWorld->CreateWorld(SPEC_BASE_NAME ".CreateWorld");
				TestTrue("TestWorld is valid", IsValid(TestWorld->World));
			});
		});

		Describe("GetWorldContext", [this]() {
			It("should point to the same world as the World pointer member", [this]() {
				TestWorld->CreateWorld(SPEC_BASE_NAME ".GetWorldContext");
				const FWorldContext& WorldContext = TestWorld->GetWorldContext();
				TestEqual("World", TestWorld->World, WorldContext.World());
			});
		});

		Describe("BeginPlay", [this]() {
			It("should initialize actors for play", [this]() {
				TestWorld->CreateWorld(SPEC_BASE_NAME ".BeginPlay");
				TestWorld->BeginPlay();
				TestTrue("AreActorsInitialized", TestWorld->World->AreActorsInitialized());
			});

			It("should fail without crash and throw an error if the world was not created prior", [this]() {
				TSharedPtr<FOUUAutomationTestWorld> LocalTempWorldContext =
					MakeShared<FOUUAutomationTestWorld>("FAutomationTestWorldSpec_NestedSpec");
				AddExpectedError("Could not send BeginPlay to invalid world!", EAutomationExpectedErrorFlags::Exact, 1);
				LocalTempWorldContext->BeginPlay();
				LocalTempWorldContext->DestroyWorld();
				LocalTempWorldContext.Reset();
			});
		});

		Describe("InitializeGame", [this]() {
			Describe("when called after world creation", [this]() {
				BeforeEach([this]() {
					TestWorld->CreateWorld(SPEC_BASE_NAME ".InitializeGame");
					const bool bInitializeResult = TestWorld->InitializeGame();
					TestTrue("initialization successful", bInitializeResult);
				});

				It("should initialize actors for play",
				   [this]() { TestTrue("AreActorsInitialized", TestWorld->World->AreActorsInitialized()); });

				It("should create a set of valid core game framework objects", [this]() {
					TestTrue("GameInstance", IsValid(TestWorld->GameInstance));
					TestTrue("GameMode", IsValid(TestWorld->GameMode));
					TestTrue("LocalPlayer", IsValid(TestWorld->LocalPlayer));
					TestTrue("PlayerController", IsValid(TestWorld->PlayerController));
				});
			});

			It("should fail without crash and throw an error if the world was not created prior", [this]() {
				TSharedPtr<FOUUAutomationTestWorld> LocalTempWorldContext =
					MakeShared<FOUUAutomationTestWorld>("FAutomationTestWorldSpec_NestedSpec");
				AddExpectedError("Could not InitializeGame invalid world!", EAutomationExpectedErrorFlags::Exact, 1);
				LocalTempWorldContext->InitializeGame();
				LocalTempWorldContext->DestroyWorld();
				LocalTempWorldContext.Reset();
			});
		});
	});

	Describe("DestroyWorld", [this]() {
		It("should reset the world to nullptr", [this]() {
			TestWorld->CreateWorld(SPEC_BASE_NAME ".DestroyWorld.01");
			TestWorld->DestroyWorld();
			TestNull("World pointer is null", TestWorld->World);
		});

		It("should reset the pointers to game framework objects that were created with InitializeGame()", [this]() {
			TestWorld->CreateWorld(SPEC_BASE_NAME "DestroyWorld.02");
			TestWorld->InitializeGame();
			TestWorld->DestroyWorld();
			TestNull("GameInstance", TestWorld->GameInstance);
			TestNull("GameMode", TestWorld->GameMode);
			TestNull("LocalPlayer", TestWorld->LocalPlayer);
			TestNull("PlayerController", TestWorld->PlayerController);
		});
	});

	AfterEach([this]() {
		// Just to make sure we don't have any memory leaks
		TestWorld->DestroyWorld();
		TestWorld.Reset();
	});
}

#endif
