// Copyright (c) 2023 Jonas Reich & Contributors

#pragma once

#include "GameplayDebugger/GameplayDebuggerCategory_OUUBase.h"

#if WITH_GAMEPLAY_DEBUGGER

class FGameplayDebuggerCanvasContext;

/**
 * Gameplay debugger to show hotkeys for switching buffer visualizations in-game.
 * Same functionality as ADebugCameraController, but within the regular game camera.
 *
 * Does not handle hiding UI (this has to be done separately).
 *
 * The advantage of using this over ADebugCameraController is in cases in which you are using custom render passes
 * or post processing that is not correctly applied to the debug camera.
 */
class OUURUNTIME_API FGameplayDebuggerCategory_ViewModes : public FOUUGameplayDebuggerCategory_Base
{
public:
	FGameplayDebuggerCategory_ViewModes();

	static auto GetCategoryName() { return TEXT("BufferVisualization"); }

	virtual void DrawData(APlayerController* InOwnerPC, FGameplayDebuggerCanvasContext& CanvasContext) override;

private:
	TSoftObjectPtr<APlayerController> OwnerPC;

	/** Whether set view mode to display GBuffer visualization overview */
	bool bEnableBufferVisualization = false;

	/** Whether set view mode to display GBuffer visualization full */
	bool bEnableBufferVisualizationFullMode = false;

	/** Last index in settings array for cycle view modes */
	uint32 LastViewModeSettingsIndex = 0;

	/** Last display enabled setting before toggling buffer visualization overview */
	bool bLastDisplayEnabled = false;

	/** Whether GBuffer visualization overview inputs are set up  */
	bool bIsBufferVisualizationInputSetup = false;

	/** Buffer selected in buffer visualization overview or full screen view */
	FString CurrSelectedBuffer;

private:
	void CycleViewMode();
	TArray<FString> GetBufferVisualizationOverviewTargets();
	void ToggleBufferVisualizationOverviewMode();
	void GetNextBuffer(int32 Step);
	void GetNextBuffer(const TArray<FString>& OverviewBuffers, int32 Step);
	void BufferVisualizationMoveUp();
	void BufferVisualizationMoveDown();
	void BufferVisualizationMoveRight();
	void BufferVisualizationMoveLeft();
	void ToggleBufferVisualizationFullMode();
	void SetBufferVisualizationFullMode(bool bFullMode);
	void SetupBufferVisualizationOverviewInput();
};
#endif
