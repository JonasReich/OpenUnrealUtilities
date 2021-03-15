// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Widgets/ActivatableWidget.h"
// UMG classes
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void CreateComplexUserWidget(FOUUAutomationTestWorld& TestWorld, UOUUActivatableWidget* Widget, bool bAddButton)
{
	UWidgetTree* WidgetTree = Widget->WidgetTree;

	UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>();
	WidgetTree->RootWidget = HorizontalBox;
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>();
	TextBlock->SetVisibility(ESlateVisibility::HitTestInvisible);
	HorizontalBox->AddChildToHorizontalBox(TextBlock);
	UImage* Image = WidgetTree->ConstructWidget<UImage>();
	Image->SetVisibility(ESlateVisibility::HitTestInvisible);
	HorizontalBox->AddChildToHorizontalBox(Image);

	// Nested UUserWidget
	UOUUTestWidget* NestedTestWidget = CreateWidget<UOUUTestWidget>(TestWorld.PlayerController);
	HorizontalBox->AddChildToHorizontalBox(NestedTestWidget);

	UHorizontalBox* NestedHorizontalBox = NestedTestWidget->WidgetTree->ConstructWidget<UHorizontalBox>();
	NestedTestWidget->WidgetTree->RootWidget = NestedHorizontalBox;

	if (bAddButton)
	{
		UButton* NestedButton = NestedTestWidget->WidgetTree->ConstructWidget<UButton>();
		NestedHorizontalBox->AddChildToHorizontalBox(NestedButton);
	}
}

BEGIN_DEFINE_SPEC(FActivatableWidgetSpec, "OpenUnrealUtilities.UMG.Widgets.ActivatableWidget", DEFAULT_OUU_TEST_FLAGS)
FOUUAutomationTestWorld TestWorld;
UOUUActivatableWidget* Widget;
END_DEFINE_SPEC(FActivatableWidgetSpec)
void FActivatableWidgetSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld.CreateWorld();
		TestWorld.InitializeGame();
		Widget = CreateWidget<UOUUActivatableWidget>(TestWorld.PlayerController);
	});

	Describe("IsActivated", [this]()
	{
		It("should return false if bIsActivatedByDefault is disabled before initialization", [this]()
		{
			UOUUActivatableWidget* UninitializedWidget = NewObject<UOUUActivatableWidget>();
			UninitializedWidget->SetPlayerContext(FLocalPlayerContext(TestWorld.LocalPlayer, TestWorld.World));
			UninitializedWidget->bIsActivatedByDefault = false;
			UninitializedWidget->Initialize();
			TestFalse("IsActivated", UninitializedWidget->IsActivated());
		});

		It("should return true by default if bIsActivatedByDefault is enabled before initialization", [this]()
		{
			UOUUActivatableWidget* UninitializedWidget = NewObject<UOUUActivatableWidget>();
			UninitializedWidget->SetPlayerContext(FLocalPlayerContext(TestWorld.LocalPlayer, TestWorld.World));
			UninitializedWidget->bIsActivatedByDefault = true;
			UninitializedWidget->Initialize();
			TestTrue("IsActivated", UninitializedWidget->IsActivated());
		});
	});

	Describe("Activate", [this]()
	{
		It("should activate the widget", [this]()
		{
			Widget->Initialize();
			TestFalse("IsActivated before activation", Widget->IsActivated());
			Widget->Activate();
			TestTrue("IsActivated after activation", Widget->IsActivated());
		});

		It("should change the visibility from Collapsed to SelfHitTestInvisible by default", [this]()
		{
			Widget->Initialize();
			TestEqual("Visibility before activation", Widget->GetVisibility(), ESlateVisibility::Collapsed);
			Widget->Activate();
			TestEqual("Visibility after activation", Widget->GetVisibility(), ESlateVisibility::SelfHitTestInvisible);
		});

		It("should change the visibility from Hidden to Visible if the visibility properties are set accordingly", [this]()
		{
			Widget->DeactivatedVisibility = ESlateVisibility::Hidden;
			Widget->ActivatedVisibility = ESlateVisibility::Visible;
			Widget->ForceUpdateVisibility();
			TestEqual("Visibility before activation", Widget->GetVisibility(), ESlateVisibility::Hidden);
			Widget->Activate();
			TestEqual("Visibility after activation", Widget->GetVisibility(), ESlateVisibility::Visible);
		});
	});

	Describe("Deactivate", [this]()
	{
		It("should deactivate a previously activated widget", [this]()
		{
			Widget->Initialize();
			Widget->Activate();
			Widget->Deactivate();
			TestFalse("IsActivated after deactivation", Widget->IsActivated());
		});

		It("should change the visibility from SelfHitTestInvisible back to Collapsed", [this]()
		{
			Widget->Initialize();
			Widget->Activate();
			Widget->Deactivate();
			TestEqual("visibility after deactivation", Widget->GetVisibility(), ESlateVisibility::Collapsed);
		});
	});

	Describe("ForceUpdateVisibility", [this]()
	{
		It("should not change the visibility after activation, as it should already be set to the correct value", [this]()
		{
			Widget->Activate();
			ESlateVisibility VisibilityAfterActivation = Widget->GetVisibility();
			Widget->ForceUpdateVisibility();
			ESlateVisibility VisibilityAfterForceUpdate = Widget->GetVisibility();
			TestEqual("Visibility", VisibilityAfterForceUpdate, VisibilityAfterActivation);
		});

		It("should reset the visibility to the correct value after it was changed externally", [this]()
		{
			Widget->ActivatedVisibility = ESlateVisibility::Visible;
			Widget->Activate();
			Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Widget->ForceUpdateVisibility();
			ESlateVisibility VisibilityAfterForceUpdate = Widget->GetVisibility();
			TestEqual("Visibility", VisibilityAfterForceUpdate, ESlateVisibility::Visible);
		});
	});

	AfterEach([this]()
	{
		TestWorld.DestroyWorld();
	});
}

#endif
