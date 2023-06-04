// Copyright (c) 2023 Jonas Reich & Contributors

#include "LatentAutomationPIEWorldLoader.h"

#if WITH_AUTOMATION_WORKER
	#include "Engine/Engine.h"
	#include "Engine/World.h"
	#include "GameFramework/GameStateBase.h"
	#include "LogOpenUnrealUtilities.h"
	#include "Misc/Paths.h"
	#include "Tests/AutomationCommon.h"
	#include "TimerManager.h"

	#if WITH_EDITOR
		#include "Editor/UnrealEdEngine.h"
		#include "UnrealEdGlobals.h"
	#endif

namespace OUU::TestUtilities
{
	namespace Private
	{
		// COPIED FROM AutomationCommon.cpp
		// @todo this is a temporary solution. Once we know how to get test's hands on a proper world
		// this function should be redone/removed
		UWorld* GetAnyGameWorld()
		{
			UWorld* TestWorld = nullptr;
			const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
			for (const FWorldContext& Context : WorldContexts)
			{
				if (((Context.WorldType == EWorldType::PIE) || (Context.WorldType == EWorldType::Game))
					&& (Context.World() != NULL))
				{
					TestWorld = Context.World();
					break;
				}
			}

			return TestWorld;
		}

	} // namespace Private

	FLatentAutomationPIEWorldLoader::FLatentAutomationPIEWorldLoader(
		FAutomationSpecBase& OwningSpec,
		FString MapName,
		bool bIgnoreLoadErrors) :
		OwningSpec(OwningSpec), MapName(MapName), bIgnoreLoadErrors(bIgnoreLoadErrors)
	{
	}

	void FLatentAutomationPIEWorldLoader::LatentLoad(const FDoneDelegate& Done)
	{
		if (MapLoadedDelegate.IsBound())
		{
			UE_LOG(
				LogOpenUnrealUtilities,
				Warning,
				TEXT("FAutomationPIEWorldLoader::MapLoadedDelegate is already bound. This may be because a "
					 "previous test didn't complete properly."));
			MapLoadedDelegate.Unbind();
		}

		AutomationOpenMap(MapName, true /* force reload */);
		MapLoadedDelegate = Done;

		Update_MapLoaded();
	}

	void FLatentAutomationPIEWorldLoader::LatentLoadBeforeEach()
	{
		OwningSpec.LatentBeforeEach([this](const FDoneDelegate& Done) { LatentLoad(Done); });
	}

	UWorld* FLatentAutomationPIEWorldLoader::GetLoadedWorld() const { return Private::GetAnyGameWorld(); }

	void FLatentAutomationPIEWorldLoader::ClosePIE()
	{
	#if WITH_EDITOR
		if (GUnrealEd)
		{
			GUnrealEd->RequestEndPlayMap();
		}
	#endif
	}

	bool FLatentAutomationPIEWorldLoader::Update_WaitForShaderToFinishCompiling()
	{
		// GShaderCompilingManager->FinishAllCompilation();
		// FAssetCompilingManager::Get().FinishAllCompilation();
		return false;
	}

	bool FLatentAutomationPIEWorldLoader::IsGameStartComplete()
	{
		UWorld* TestWorld = GetLoadedWorld();

		if (TestWorld && TestWorld->AreActorsInitialized())
		{
			AGameStateBase* GameState = TestWorld->GetGameState();
			if (GameState && GameState->HasMatchStarted())
			{
				// remove any paths or extensions to match the name of the world
				FString ShortMapName = FPackageName::GetShortName(MapName);
				ShortMapName = FPaths::GetBaseFilename(ShortMapName);

				// Handle both ways the user may have specified this
				if (TestWorld->GetName() == ShortMapName)
				{
					return true;
				}
			}
		}

		return false;
	}

	void FLatentAutomationPIEWorldLoader::Update_MapLoaded()
	{
		if (UWorld* GameWorld = GetLoadedWorld())
		{
			GameWorld->GetTimerManager().SetTimerForNextTick([this]() {
				if (IsGameStartComplete())
				{
					if (bIgnoreLoadErrors)
					{
						// Ignore all errors that occured during map load up to this point
						OwningSpec.ClearExecutionInfo();
					}
					MapLoadedDelegate.Execute();
					MapLoadedDelegate.Unbind();
					return;
				}
				Update_MapLoaded();
			});
		}
		else
		{
			OwningSpec.AddError("No game world found for Automation PIE");
			MapLoadedDelegate.Execute();
			MapLoadedDelegate.Unbind();
		}
	}
} // namespace OUU::TestUtilities

#endif
