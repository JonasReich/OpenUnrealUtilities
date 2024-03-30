// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_WORKER

namespace OUU::TestUtilities
{
	/**
	 * Utility for loading maps into automation PIE just like functional tests.
	 *
	 * Usage is primarily intended for automation specs that can't use FAutomationLatentCommands directly.
	 * Example:
	 *
	 *     BEGIN_DEFINE_SPEC(FSampleSpec, "Spec.Path", DEFAULT_OUU_TEST_FLAGS)
	 *         FLatentAutomationPIEWorldLoader WorldLoader {*this};
	 *     END_DEFINE_SPEC(FSampleSpec)
	 *
	 *     void FSampleSpec::Define()
	 *     {
	 *         LatentBeforeEach([this](const FDoneDelegate& Done) {
	 *             WorldLoader.LatentLoad(Done);
	 *         });
	 *
	 *         Describe("Your feature", [this]() {
	 *             It("should do something", [this]() {
	 *                 UWorld* World = WorldLoader.GetLoadedWorld();
	 *                 // ...
	 *             }
	 *         }
	 *     }
	 */
	class OUUTESTUTILITIES_API FLatentAutomationPIEWorldLoader
	{
	public:
		FLatentAutomationPIEWorldLoader(
			FAutomationSpecBase& OwningSpec,
			const FString& MapName = TEXT("/OpenUnrealUtilities/Runtime/EmptyWorld"),
			bool bIgnoreLoadErrors = true);

		void LatentLoad(const FDoneDelegate& Done);

		void LatentLoadBeforeEach();
		
		UWorld* GetLoadedWorld() const;

		// Close PIE (if in editor). Put this in your test cleanup, otherwise the PIE window remains open.
		void ClosePIE();

	private:
		FAutomationSpecBase& OwningSpec;
		FDoneDelegate MapLoadedDelegate;
		const FString MapName;
		bool bIgnoreLoadErrors;

		bool IsGameStartComplete() const;

		void Update_MapLoaded();
	};
} // namespace OUU::TestUtilities

#endif
