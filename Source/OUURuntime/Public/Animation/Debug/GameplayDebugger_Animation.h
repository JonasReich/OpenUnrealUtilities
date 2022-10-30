// Copyright (c) 2022 Jonas Reich

#pragma once

#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

#if WITH_GAMEPLAY_DEBUGGER
	#include "CoreMinimal.h"
	#include "Animation/AnimInstance.h"
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

	virtual void DrawData(APlayerController* OwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

private:
	int32 DebugMeshComponentIndex = 0;
	int32 DebugInstanceIndex = -1;

	FGraphTraversalCounter DebugDataCounter;

	void CycleDebugMesh();
	void CycleDebugInstance();

	void DrawSceneComponentTree(
		FGameplayDebuggerCanvasContext& CanvasContext,
		const AActor* DebugActor,
		USkeletalMeshComponent* DebugMeshComponent) const;

	void DisplayDebug(
		FGameplayDebuggerCanvasContext& CanvasContext,
		USkeletalMeshComponent* SkeletalMeshComponent,
		UOUUDebuggableAnimInstance* AnimInstance,
		UCanvas* Canvas);

	void DisplayDebugInstance(
		FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
		USkeletalMeshComponent* SkeletalMeshComponent,
		UOUUDebuggableAnimInstance* AnimInstance,
		float& Indent);

	void OutputTickRecords(
		const TArray<FAnimTickRecord>& Records,
		UCanvas* Canvas,
		float Indent,
		const int32 HighlightIndex,
		FLinearColor TextColor,
		FLinearColor HighlightColor,
		FLinearColor InactiveColor,
		FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
		bool bFullBlendspaceDisplay);

	void OutputCurveMap(
		TMap<FName, float>& CurveMap,
		FGameplayDebugger_DisplayDebugManager& DisplayDebugManager,
		float Indent);
};

#endif // WITH_GAMEPLAY_DEBUGGER
