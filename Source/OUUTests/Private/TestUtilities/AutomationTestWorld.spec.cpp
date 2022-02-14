// Copyright (c) 2021 Jonas Reich

#pragma once

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

	#include "AutomationTestWorld.h"
	#include "GameFramework/GameModeBase.h"

BEGIN_DEFINE_SPEC(
	FAutomationTestWorldSpec,
	"OpenUnrealUtilities.TestUtilities.AutomationTestWorld",
	DEFAULT_OUU_TEST_FLAGS)
	TSharedPtr<FOUUAutomationTestWorld> TestWorld;
END_DEFINE_SPEC(FAutomationTestWorldSpec)
void FAutomationTestWorldSpec::Define()
{
	BeforeEach([this]() { TestWorld = MakeShared<FOUUAutomationTestWorld>("FAutomationTestWorldSpec"); });

	Describe("", [this]() {
		Describe("CreateWorld", [this]() {
			It("should create a valid test world", [this]() {
				TestWorld->CreateWorld();
				TestTrue("TestWorld is valid", IsValid(TestWorld->World));
			});
		});

		Describe("GetWorldContext", [this]() {
			It("should point to the same world as the World pointer member", [this]() {
				TestWorld->CreateWorld();
				FWorldContext& WorldContext = TestWorld->GetWorldContext();
				TestEqual("World", TestWorld->World, WorldContext.World());
			});
		});

		Describe("BeginPlay", [this]() {
			It("should initialize actors for play", [this]() {
				TestWorld->CreateWorld();
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

		Describe("InitiailizeGame", [this]() {
			Describe("when called after world creation", [this]() {
				BeforeEach([this]() {
					TestWorld->CreateWorld();
					bool bInitializeResult = TestWorld->InitializeGame();
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
				AddExpectedError("Could not InitiailizeGame invalid world!", EAutomationExpectedErrorFlags::Exact, 1);
				LocalTempWorldContext->InitializeGame();
				LocalTempWorldContext->DestroyWorld();
				LocalTempWorldContext.Reset();
			});
		});
	});

	Describe("DestroyWorld", [this]() {
		It("should reset the world to nullptr", [this]() {
			TestWorld->CreateWorld();
			TestWorld->DestroyWorld();
			TestNull("World pointer is null", TestWorld->World);
		});

		It("should reset the pointers to game framework objects that were created with InitializeGame()", [this]() {
			TestWorld->CreateWorld();
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
