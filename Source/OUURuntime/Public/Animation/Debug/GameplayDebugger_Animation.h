// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

#if WITH_GAMEPLAY_DEBUGGER
	#include "CoreMinimal.h"
	#include "GameplayDebuggerCategory.h"

class APlayerController;
class UCanvas;
class UOUUDebuggableAnimInstance;

struct FAnimInstanceProxy;
struct FGameplayDebugger_DisplayDebugManager;

/**
 * Gameplay debugger for animation system.
 * Extracted/extended functionality from "ShowDebug Animation" command.
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
		USkeletalMeshComponent* SkeletalMeshComponent,
		UOUUDebuggableAnimInstance* AnimInstance,
		UCanvas* Canvas);

	static void DisplayDebugInstance(
		FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
		const USkeletalMeshComponent* SkeletalMeshComponent,
		UOUUDebuggableAnimInstance* AnimInstance,
		const float& Indent);

	static void OutputTickRecords(
		const TArray<FAnimTickRecord>& Records,
		UCanvas* Canvas,
		float Indent,
		const int32 HighlightIndex,
		FLinearColor TextColor,
		FLinearColor HighlightColor,
		FLinearColor InactiveColor,
		FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
		bool bFullBlendSpaceDisplay);

	static void OutputCurveMap(
		TMap<FName, float>& CurveMap,
		FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
		float Indent);
};

#endif // WITH_GAMEPLAY_DEBUGGER
