// Copyright (c) 2023 Jonas Reich & Contributors

#include "GameplayDebugger/GameplayDebuggerExtension_ActorSelect.h"

#if WITH_GAMEPLAY_DEBUGGER

#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameplayDebuggerCategoryReplicator.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"

FGameplayDebuggerExtension_ActorSelect::FGameplayDebuggerExtension_ActorSelect()
{
	auto CtrlAndAltModifier = FGameplayDebuggerInputModifier::Ctrl;
	CtrlAndAltModifier.bAlt = true;

	BindKeyPress(
		TEXT("Select Local Player Pawn"),
		EKeys::Home.GetFName(),
		CtrlAndAltModifier,
		this,
		&FGameplayDebuggerExtension_ActorSelect::SelectLocalPlayerPawn);

	BindKeyPress(
		TEXT("Select Closest NPC"),
		EKeys::End.GetFName(),
		CtrlAndAltModifier,
		this,
		&FGameplayDebuggerExtension_ActorSelect::SelectClosestNPC);

	#define BIND_SELECT_PLAYER_X(Digit, Number)                                                                        \
		BindKeyPress(                                                                                                  \
			TEXT("Select Player " PREPROCESSOR_TO_STRING(Digit)),                                                      \
			EKeys::Number.GetFName(),                                                                                  \
			CtrlAndAltModifier,                                                                                        \
			this,                                                                                                      \
			&FGameplayDebuggerExtension_ActorSelect::SelectPlayerPawn_##Digit)

	BIND_SELECT_PLAYER_X(1, One);
	BIND_SELECT_PLAYER_X(2, Two);
	BIND_SELECT_PLAYER_X(3, Three);
	BIND_SELECT_PLAYER_X(4, Four);

	#undef BIND_SELECT_PLAYER_X
}

void FGameplayDebuggerExtension_ActorSelect::SelectLocalPlayerPawn()
{
	if (auto* LocalController = UGameplayStatics::GetPlayerController(GetWorldFromReplicator(), 0))
	{
		auto* LocalPawn = LocalController->GetPawn();
		GetReplicator()->SetDebugActor(
			LocalPawn ? ImplicitConv<AActor*>(LocalPawn) : ImplicitConv<AActor*>(LocalController));
	}
}

void FGameplayDebuggerExtension_ActorSelect::SelectClosestNPC()
{
	if (auto* LocalController = UGameplayStatics::GetPlayerController(GetWorldFromReplicator(), 0))
	{
		auto* LocalPawn = LocalController->GetPawn();
		auto* ReferenceActor = LocalPawn ? ImplicitConv<AActor*>(LocalPawn) : ImplicitConv<AActor*>(LocalController);
		FVector ReferenceLocation = ReferenceActor->GetActorLocation();
		APawn* ClosestPawn = nullptr;
		float ClosestDist = TNumericLimits<float>::Max();
		for (APawn* Pawn : TActorRange<APawn>(GetWorldFromReplicator()))
		{
			if (Pawn->IsPlayerControlled())
				continue;

			float DistSquared = FVector::DistSquared(ReferenceLocation, Pawn->GetActorLocation());
			if (DistSquared > ClosestDist)
				continue;

			ClosestDist = DistSquared;
			ClosestPawn = Pawn;
		}
		if (ClosestPawn)
		{
			GetReplicator()->SetDebugActor(ClosestPawn);
		}
	}
}

	#define DEF_SELECT_PLAYER_X(Digit)                                                                                 \
		void FGameplayDebuggerExtension_ActorSelect::SelectPlayerPawn_##Digit() { SelectPlayerPawn_X(Digit); }
DEF_SELECT_PLAYER_X(1)
DEF_SELECT_PLAYER_X(2)
DEF_SELECT_PLAYER_X(3)
DEF_SELECT_PLAYER_X(4)
	#undef DEF_SELECT_PLAYER_X

void FGameplayDebuggerExtension_ActorSelect::SelectPlayerPawn_X(int32 X)
{
	// Convert from "normal" counting to 0 based indices
	const int32 PlayerStateIndex = X - 1;
	if (auto* PlayerState = UGameplayStatics::GetPlayerState(GetWorldFromReplicator(), PlayerStateIndex))
	{
		auto* Pawn = PlayerState->GetPawn();
		GetReplicator()->SetDebugActor(Pawn ? ImplicitConv<AActor*>(Pawn) : ImplicitConv<AActor*>(PlayerState));
	}
}

#endif
