// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"
#if WITH_GAMEPLAY_DEBUGGER
	#include "Animation/AnimationAsset.h"
	#include "Animation/AnimTypes.h"
	#include "CoreMinimal.h"
	#include "GameplayDebuggerCategory.h"

class APlayerController;
class UCanvas;
class UOUUDebuggableAnimInstance;

struct FAnimInstanceProxy;

/**
 * Gameplay debugger for animation system.
 * Extended functionality from "ShowDebug Animation" command.
 */
class OUURUNTIME_API FGameplayDebuggerCategory_Animation : public FOUUGameplayDebuggerCategory_Base
{
public:
	FGameplayDebuggerCategory_Animation();

	static auto GetCategoryName() { return TEXT("Animation"); }

	void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

private:
	int32 DebugMeshComponentIndex = 0;
	int32 DebugInstanceIndex = -1;

	FGraphTraversalCounter DebugDataCounter;

	void CycleDebugMesh();
	void CycleDebugInstance();

	static void DrawSceneComponentTree(
		FGameplayDebuggerCanvasContext& CanvasContext,
		const AActor* DebugActor,
		const USkeletalMeshComponent* DebugMeshComponent);

	void DisplayDebug(
		FGameplayDebuggerCanvasContext& CanvasContext,
		UOUUDebuggableAnimInstance* AnimInstance,
		UCanvas* Canvas);
};

#endif // WITH_GAMEPLAY_DEBUGGER
