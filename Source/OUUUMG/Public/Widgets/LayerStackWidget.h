// Copyright (c) 2021 Jonas Reich

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LayerStackWidget.generated.h"

class UOverlay;
class UOUULayerWidget;

/**
 * Layer stack widgets allow organizing all of the UI of a game into activatable layers that manage
 * visibility and interactibility dynamically based on layer activation.
 * 
 * When used, LayerStackWidgets must be the only widgets added directly to the Player viewport to ensure
 * that no other widgets can interfere with input and focus handling.
 * One basic principle is that only one layer can ever be interactable at a time, which means all UI elements
 * that should be interactable simultaneously must be grouped into the same layer.
 * 
 * Using more than one stack is possible, but adding them to the screen should be done indirectly with the
 * Link functions instead of adding the stack directly to the screen. Separating the UI into different stack
 * is a useful approach that should be taken when  switching between bigger contexts (e.g. based on game mode).
 */
UCLASS()
class OUUUMG_API UOUULayerStackWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UOUULayerStackWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * This does the same as AddToPlayerScreen(), but the semantics are a bit more specific and tell the reader
	 * that the stack was put to the screen with the intention of adding additional stack later on with the linking functions below.
	 */
	void StartNewLinkedStack();
	void InsertToLinkedStackAbove(UOUULayerStackWidget* OtherStack);
	void InsertToLinkedStackBelow(UOUULayerStackWidget* OtherStack);
	void RemoveFromLinkedStack();

	bool IsLinkedStackHead() const;
	bool IsLinkedStackRoot() const;
	UOUULayerStackWidget* GetStackAbove() const;
	UOUULayerStackWidget* GetStackBelow() const;
	UOUULayerStackWidget* GetLinkedStackHead();
	UOUULayerStackWidget* GetLinkedStackRoot();

protected:
	/**
	 * The widget overlay stack that contains the layer widgets.
	 * Layer widgets should only be added/removed/reordered during design-time in the UMG designer.
	 */
	UPROPERTY(meta=(BindWidget))
	UOverlay* WidgetStack;

	// - UUserWidget
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	// --

private:
	// This flag is used to verify if the linked stack was created correctly with StartNewLinkedStack() or one of the Insert functions,
	// or if the stack was just added to the viewport.
	bool bAddedCorrectly = false;

	TWeakObjectPtr<UOUULayerStackWidget> LinkedStackAbove;
	TWeakObjectPtr<UOUULayerStackWidget> LinkedStackBelow;
	
	void InsertBetween(UOUULayerStackWidget* NewAbove, UOUULayerStackWidget* NewBelow);

	void UpdateLayers(UOUULayerWidget* BottomLayerFromStackAbove);

	UFUNCTION()
	void HandleRequestResetFocusFromTopLayer();

	void ResetUserFocusRecursively();

	void AddRecursivelyToPlayerScreenFromRoot();

	void AddRecursivelyToPlayerScreen(int32 ZOrder);
};
