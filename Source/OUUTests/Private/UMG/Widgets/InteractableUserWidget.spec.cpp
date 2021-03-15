// Copyright (c) 2021 Jonas Reich

#include "OUUTestUtilities.h"

#if WITH_AUTOMATION_WORKER

#include "Widgets/InteractableWidget.h"
#include "UMG/Widgets/InteractableUserWidgetTests.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"

BEGIN_DEFINE_SPEC(FInteractableUserWidgetSpec, "OpenUnrealUtilities.UMG.Widgets.InteractableUserWidget", DEFAULT_OUU_TEST_FLAGS)
FOUUAutomationTestWorld TestWorld;
UOUUInteractableWidget* Widget;
UWidgetTree* WidgetTree;
END_DEFINE_SPEC(FInteractableUserWidgetSpec)
void FInteractableUserWidgetSpec::Define()
{
	BeforeEach([this]()
	{
		TestWorld.CreateWorld();
		TestWorld.InitializeGame();
		Widget = CreateWidget<UOUUInteractableWidget>(TestWorld.PlayerController);
		WidgetTree = Widget->WidgetTree;
	});

	Describe("TryResetUserFocusTo", [this]()
	{
		It("should fail when called on nullptr", [this]()
		{
			SPEC_TEST_FALSE(IUserFocusResetableWidget::TryResetUserFocusTo(nullptr));
		});

		It("should fail to set the focus when the target widget is a focusable button, because it does not implement the interface", [this]()
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>();
			Button->IsFocusable = true;
			auto SlateWidget = Button->TakeWidget();
			SPEC_TEST_FALSE(IUserFocusResetableWidget::TryResetUserFocusTo(Button));
		});

		It("should fail to set the focus when the target widget is a NOT focusable button", [this]()
		{
			UButton* Button = WidgetTree->ConstructWidget<UButton>();
			Button->IsFocusable = false;
			auto SlateWidget = Button->TakeWidget();
			SPEC_TEST_FALSE(IUserFocusResetableWidget::TryResetUserFocusTo(Button));
		});

		It("should successfully set the focus when there is a focusable button inside a UUserWidget", [this]()
		{
			UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>();
			WidgetTree->RootWidget = HorizontalBox;
			UButton* Button = WidgetTree->ConstructWidget<UButton>();
			HorizontalBox->AddChildToHorizontalBox(Button);
			Button->IsFocusable = true;
			auto SlateWidget = Widget->TakeWidget();
			SPEC_TEST_TRUE(IUserFocusResetableWidget::TryResetUserFocusTo(Widget));
		});

		It("should fail to set the focus when the target is an empty UUserWidget that is not focusable itself", [this]()
		{
			Widget->bIsFocusable = false;
			auto SlateWidget = Widget->TakeWidget();
			SPEC_TEST_FALSE(IUserFocusResetableWidget::TryResetUserFocusTo(Widget));
		});

		It("should successfully set the focus when the target is an empty UUserWidget that is focusable itself", [this]()
		{
			Widget->bIsFocusable = true;
			auto SlateWidget = Widget->TakeWidget();
			SPEC_TEST_TRUE(IUserFocusResetableWidget::TryResetUserFocusTo(Widget));
		});

		Describe("called on a UUserWidget that contains another UUserWidget which implements IUserFocusResetableWidget", [this]()
		{
			It("should successfully set the focus if ResetUserFocus_Implementation in the nested widget returns true", [this]()
			{
				UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>();
				WidgetTree->RootWidget = HorizontalBox;
				UMockFocusResetable_True* NestedWidget = WidgetTree->ConstructWidget<UMockFocusResetable_True>();
				HorizontalBox->AddChildToHorizontalBox(NestedWidget);
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_TRUE(IUserFocusResetableWidget::TryResetUserFocusTo(Widget));
			});

			It("should fail to set the focus if ResetUserFocus_Implementation in the nested widget returns false", [this]()
			{
				UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>();
				WidgetTree->RootWidget = HorizontalBox;
				UMockFocusResetable_False* NestedWidget = WidgetTree->ConstructWidget<UMockFocusResetable_False>();
				HorizontalBox->AddChildToHorizontalBox(NestedWidget);
				auto SlateWidget = Widget->TakeWidget();
				SPEC_TEST_FALSE(IUserFocusResetableWidget::TryResetUserFocusTo(Widget));
			});
		});
	});

	AfterEach([this]()
	{
		TestWorld.DestroyWorld();
	});
}

#endif
