// Copyright (c) 2021 Jonas Reich

#include "UMG/LayerStackWidget.h"
#include "UMG/LayerWidget.h"
#include "Components/Overlay.h"
#include "LayerStackWidgetTests.generated.h"

// Test layer stack that doesn't have any layers
UCLASS(meta = (Hidden, HideDropDown))
class UTestUOUULayerStackWidget_Empty : public UOUULayerStackWidget
{
	GENERATED_BODY()
protected:
	void NativeOnInitialized() override
	{
		WidgetStack = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = WidgetStack;

		Super::NativeOnInitialized();
	}
};

// Test layer that tracks the last layer above that was passed via UpdateLayer() 
UCLASS(meta = (Hidden, HideDropDown))
class UOUULayerStackTickTestLayer : public UOUULayerWidget
{
	GENERATED_BODY()
public:
	// params: self layer, last layer above
	DECLARE_EVENT_TwoParams(UOUULayerStackTickTestLayer, FOnLayerUpdated, const UOUULayerWidget*, const UOUULayerWidget*);
	FOnLayerUpdated OnLayerUpdated;

	virtual void UpdateLayer(const UOUULayerWidget* LayerAbove) override
	{
		OnLayerUpdated.Broadcast(this, LayerAbove);
		Super::UpdateLayer(LayerAbove);
	}
};

// Test layer stack that has three layers (top-down: top, middle, bottom)
UCLASS(meta = (Hidden, HideDropDown))
class UTestUOUULayerStackWidget : public UOUULayerStackWidget
{
	GENERATED_BODY()
public:
	UOUULayerStackTickTestLayer* TopLayer;
	UOUULayerStackTickTestLayer* MiddleLayer;
	UOUULayerStackTickTestLayer* BottomLayer;

	static TArray<const UOUULayerWidget*> UpdatedLayers;
	static TArray<const UOUULayerWidget*> LayersAboveUpdatedLayers;

	static void ClearUpdatedLayers()
	{
		UpdatedLayers.Empty();
		LayersAboveUpdatedLayers.Empty();
	}

protected:
	UOUULayerStackTickTestLayer* CreateLayer()
	{
		auto* NewLayer = WidgetTree->ConstructWidget<UOUULayerStackTickTestLayer>();
		WidgetStack->AddChildToOverlay(NewLayer);
		NewLayer->OnLayerUpdated.AddUObject(this, &UTestUOUULayerStackWidget::HandleLayerUpdated);
		return NewLayer;
	}

	void NativeOnInitialized() override
	{
		// This would normally happen in the blueprint designer, not at runtime!
		WidgetStack = WidgetTree->ConstructWidget<UOverlay>();
		WidgetTree->RootWidget = WidgetStack;
		TopLayer = CreateLayer();
		MiddleLayer = CreateLayer();
		BottomLayer = CreateLayer();

		Super::NativeOnInitialized();
	}

	UFUNCTION()
	void HandleLayerUpdated(const UOUULayerWidget* LayerUpdated, const UOUULayerWidget* LayerAbove)
	{
		UpdatedLayers.Add(LayerUpdated);
		LayersAboveUpdatedLayers.Add(LayerAbove);
	}
};
