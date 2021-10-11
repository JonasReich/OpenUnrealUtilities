// Copyright (c) 2021 Jonas Reich

#include "CoreMinimal.h"

#include "LogOpenUnrealUtilities.h"

class FActorMapWindowBootstrapper
{
public:
	UWorld* TargetWorld = nullptr;

	DECLARE_EVENT(FActorMapWindowBootstrapper, FOnWindowClosed);
	FOnWindowClosed OnWindowClosed;

	static UWorld* GetCurrentTargetWorld()
	{
		// Always prefer the play world (both in cooked game and in PIE)
		if (UWorld* PossibleResult = GEngine->GetCurrentPlayWorld())
			return PossibleResult;

#if WITH_EDITOR
		if (GIsEditor)
		{
			// Fallback to the editor world in the editor
			return GEditor->GetEditorWorldContext().World();
		}
#endif
		return nullptr;
	}

	void OpenWindowForCurrentWorld()
	{
		checkf(!IsValid(TargetWorld), TEXT("OpenWindowForCurrentWorld() must not be called twice on the same object"));

		TargetWorld = GetCurrentTargetWorld();
		GEngine->OnWorldDestroyed().AddRaw(this, &FActorMapWindowBootstrapper::HandleWorldDestroyed);
	}

	void HandleWorldDestroyed(UWorld* WorldDestroyed)
	{
		if (!ensure(IsValid(WorldDestroyed)))
			return;

		// Automatically close the window once the world is destroyed.
		if (WorldDestroyed == TargetWorld)
			CloseWindow();
	}

	void CloseWindow()
	{
		// #TODO-j.reich actually close window
		OnWindowClosed.Broadcast();
	}
};

// Use unique pointer for now, so we only have to support a single window using the cheat.
TUniquePtr<FActorMapWindowBootstrapper> ActorMapWindowBootstrapper;

void OpenActorMapWindowForCurrentWorld()
{
	if (ActorMapWindowBootstrapper.IsValid())
	{
		UE_LOG(LogOpenUnrealUtilities, Warning, TEXT("OpenActorMapWindowForCurrentWorld() was called, but a different window should already be opened. Ignoring this call..."))
		return;
	}
	ActorMapWindowBootstrapper = MakeUnique<FActorMapWindowBootstrapper>();
	// Automatically clean up unique pointer once the window is closed.
	ActorMapWindowBootstrapper->OnWindowClosed.AddLambda([]()
	{
		ActorMapWindowBootstrapper.Reset();
	});
	ActorMapWindowBootstrapper->OpenWindowForCurrentWorld();
}

